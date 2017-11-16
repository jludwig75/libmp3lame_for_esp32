#include "serial_port.h"

#include <vector>

#include <Windows.h>
#include <assert.h>

#include "utils.h"

using namespace std;


uint8_t expected_simulation_start_sequence[9] = { 0x55, 0xAA, 'S', 'T', 'A', 'R', 'T', 0x55, 0xAA };


SerialPort::SerialPort(const std::string & port_name, bool port_simulated_from_file) :
    _name(port_name),
    _port_simulated_from_file(port_simulated_from_file),
    _handle(INVALID_HANDLE_VALUE),
    _stop_reader_thread(false),
    _thread_handle(NULL),
    _total_bytes_stored(0),
    _total_bytes_read(0)
{
    if (_port_simulated_from_file)
    {
        // Stuff garbage into the serial read buffer to simulated initial garbage
        for (int i = 0; i < 12 + rand() % 12; i++)
        {
            uint8_t byte = static_cast<uint8_t>(rand() % UINT8_MAX);
            store_bytes(&byte, sizeof(byte));
        }

        // Now send the start sequence.
        store_bytes(expected_simulation_start_sequence, sizeof(expected_simulation_start_sequence));
    }
}

SerialPort::~SerialPort()
{
    if (_thread_handle)
    {
        stop_reader_thread();
    }

    if (_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_handle);
    }
}

const std::string & SerialPort::name() const
{
    return _name;
}

bool SerialPort::open()
{
    _handle = CreateFileA(_name.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);
    if (_handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    if (!_port_simulated_from_file)
    {
        DCB dcb;
        FillMemory(&dcb, sizeof(dcb), 0);
        if (!GetCommState(_handle, &dcb))     // get current DCB
        {
            // Error in GetCommState
            return false;
        }

        // Update DCB rate.
        dcb.BaudRate = CBR_256000;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;

        // Set new state.
        if (!SetCommState(_handle, &dcb))
        {
            // Error in SetCommState. Possibly a problem with the communications 
            // port handle or a problem with the DCB structure itself.
            return false;
        }
    }

    _thread_handle = CreateThread(NULL,
        0,
        (PTHREAD_START_ROUTINE)read_from_port_thread_launch,
        this,
        0,
        NULL);
    if (!_thread_handle)
    {
        return false;
    }

    return true;
}

bool SerialPort::read_bytes(uint8_t * buffer, size_t buffer_byte_count, size_t & bytes_read) const
{
    unique_lock<mutex> lock(_data_buffer_lock);

    size_t bytes_to_copy = min(buffer_byte_count, _data_buffer.size());

    if (bytes_to_copy == 0)
    {
        bytes_read = 0;
        return true;
    }

    memcpy(buffer, &_data_buffer[0], bytes_to_copy);
    _data_buffer.erase(_data_buffer.begin(), _data_buffer.begin() + bytes_to_copy);
    printf("Removing %u bytes from buffer. %u remaining\n", bytes_to_copy, _data_buffer.size());

    if (_port_simulated_from_file)
    {
        convert_samples_in_place(reinterpret_cast<uint16_t *>(buffer), bytes_to_copy / sizeof(uint16_t));
    }

    bytes_read = bytes_to_copy;
    _total_bytes_read += bytes_to_copy;

    return true;
}

bool SerialPort::read_samples(uint16_t * buffer, size_t buffer_entry_count, size_t & samples_read) const
{
    size_t bytes_read = 0;
    bool ret = read_bytes(reinterpret_cast<uint8_t *>(buffer), buffer_entry_count * sizeof(uint16_t), bytes_read);

    samples_read = bytes_read / sizeof(uint16_t);
    return ret;
}

void SerialPort::read_from_port_thread_launch(SerialPort * _this)
{
    _this->read_from_port_thread();
}

#define READ_TIMEOUT      500      // milliseconds

void SerialPort::read_from_port_thread()
{
    vector<uint8_t> buffer(4093, 0);

    DWORD dwRead;
    BOOL fWaitingOnRead = FALSE;
    OVERLAPPED osReader = { 0 };

    // Create the overlapped event. Must be closed before exiting
    // to avoid a handle leak.
    osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (osReader.hEvent == NULL)
    {
        // Error creating overlapped event; abort.
        assert(0);
    }

    while (!_stop_reader_thread)
    {
        if (!fWaitingOnRead)
        {
            // Issue read operation.
            if (!ReadFile(_handle, &buffer[0], buffer.size(), &dwRead, &osReader))
            {
                if (GetLastError() != ERROR_IO_PENDING)     // read not delayed?
                {
                    // Error in communications; report it.
                    assert(0);
                }
                else
                {
                    fWaitingOnRead = TRUE;
                }
            }
            else
            {
                // read completed immediately
                osReader.Offset += dwRead;
                store_bytes(&buffer[0], dwRead);
            }
        }

        DWORD dwRes;

        if (fWaitingOnRead) {
            dwRes = WaitForSingleObject(osReader.hEvent, READ_TIMEOUT);
            switch (dwRes)
            {
                // Read completed.
            case WAIT_OBJECT_0:
                if (!GetOverlappedResult(_handle, &osReader, &dwRead, FALSE))
                {
                    // Error in communications; report it.
                    DWORD error = GetLastError();
                    if (error == ERROR_HANDLE_EOF)
                    {
                        break;
                    }

                    assert(0);
                }
                else
                {
                    // Read completed successfully.
                    osReader.Offset += dwRead;
                    store_bytes(&buffer[0], dwRead);
                }

                //  Reset flag so that another opertion can be issued.
                fWaitingOnRead = FALSE;
                break;

            case WAIT_TIMEOUT:
                // Operation isn't complete yet. fWaitingOnRead flag isn't
                // changed since I'll loop back around, and I don't want
                // to issue another read until the first one finishes.
                //
                // This is a good time to do some background work.
                break;

            default:
                // Error in the WaitForSingleObject; abort.
                // This indicates a problem with the OVERLAPPED structure's
                // event handle.
                break;
            }
        }
    }
}

void SerialPort::stop_reader_thread()
{
    _stop_reader_thread = true;
    WaitForSingleObject(_thread_handle, INFINITE);
}


void SerialPort::store_bytes(const uint8_t *data, size_t bytes)
{
    unique_lock<mutex> lock(_data_buffer_lock);

    copy(reinterpret_cast<const uint8_t *>(data), reinterpret_cast<const uint8_t *>(data) + bytes, back_inserter(_data_buffer));

    _total_bytes_stored += bytes;
    printf("Inserting %u bytes into buffer. Buffer now has %u bytes\n", bytes, _data_buffer.size());
}

void SerialPort::convert_samples_in_place(uint16_t * buffer, size_t num_samples) const
{
    int16_t *output_buffer = reinterpret_cast<int16_t *>(buffer);
    for (size_t i = 0; i < num_samples; i++)
    {
        output_buffer[i] = map<int16_t>(buffer[i], INT16_MIN, INT16_MAX, 0, 2 * 1852);
    }
}
