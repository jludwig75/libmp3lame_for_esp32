#include "serial_port.h"

#include <vector>

#include <Windows.h>
#include <assert.h>

#include "utils.h"

using namespace std;


SerialPort::SerialPort(const std::string & port_name, bool port_simulated_from_file) :
    _name(port_name),
    _port_simulated_from_file(port_simulated_from_file),
    _handle(INVALID_HANDLE_VALUE),
    _stop_reader_thread(false),
    _thread_handle(NULL),
    _total_samples_stored(0),
    _total_samples_read(0)
{

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

bool SerialPort::read_samples(uint16_t * buffer, size_t buffer_entry_count, size_t & samples_read) const
{
    unique_lock<mutex> lock(_sample_buffer_lock);

    size_t entries_to_copy = min(buffer_entry_count, _sample_buffer.size());

    if (entries_to_copy == 0)
    {
        samples_read = 0;
        return true;
    }

    memcpy(buffer, &_sample_buffer[0], entries_to_copy * sizeof(uint16_t));
    _sample_buffer.erase(_sample_buffer.begin(), _sample_buffer.begin() + entries_to_copy);
    printf("Removing %u entries from buffer. %u remaining\n", entries_to_copy, _sample_buffer.size());

    if (_port_simulated_from_file)
    {
        convert_samples_in_place(buffer, entries_to_copy);
    }

    samples_read = entries_to_copy;
    _total_samples_read += entries_to_copy;

    return true;
}

void SerialPort::read_from_port_thread_launch(SerialPort * _this)
{
    _this->read_from_port_thread();
}

#define READ_TIMEOUT      500      // milliseconds

void SerialPort::read_from_port_thread()
{
    vector<uint8_t> buffer(4096, 0);

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
                store_samples(&buffer[0], dwRead);
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
                    store_samples(&buffer[0], dwRead);
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


void SerialPort::store_samples(const uint8_t *data, size_t bytes)
{
    unique_lock<mutex> lock(_sample_buffer_lock);

    assert(bytes % 2 == 0);

    size_t samples = bytes / sizeof(uint16_t);
    copy(reinterpret_cast<const uint16_t *>(data), reinterpret_cast<const uint16_t *>(data)+samples, back_inserter(_sample_buffer));

    _total_samples_stored += samples;

    printf("Inserting %u entries to buffer\n", samples);
}

void SerialPort::convert_samples_in_place(uint16_t * buffer, size_t num_samples) const
{
    int16_t *output_buffer = reinterpret_cast<int16_t *>(buffer);
    for (size_t i = 0; i < num_samples; i++)
    {
        output_buffer[i] = map<int16_t>(buffer[i], INT16_MIN, INT16_MAX, 0, 2 * 1852);
    }
}
