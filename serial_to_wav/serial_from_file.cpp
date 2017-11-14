#include "serial_from_file.h"

#include "utils.h"

#include <io.h>
#include <fcntl.h>


SerialFromFile::SerialFromFile(const std::string & file_name, uint16_t adc_center_value) :
    _handle(-1),
    _name(file_name),
    _adc_center_value(adc_center_value)
{
}

SerialFromFile::~SerialFromFile()
{
    if (_handle >= 0)
    {
        _close(_handle);
    }
}

bool SerialFromFile::open()
{
    errno_t err = _sopen_s(&_handle, _name.c_str(), O_BINARY | O_RDONLY, _SH_DENYRW, 0);
    return err == 0;
}

static void convert_samples_in_place(uint16_t * buffer, size_t num_samples, uint16_t adc_center_value)
{
    int16_t *output_buffer = reinterpret_cast<int16_t *>(buffer);
    for (size_t i = 0; i < num_samples; i++)
    {
        output_buffer[i] = map<int16_t>(buffer[i], INT16_MIN, INT16_MAX, 0, 2 * adc_center_value);
    }
}

// Non-blocking call to read all available samples
bool SerialFromFile::read_samples(uint16_t * buffer, size_t buffer_entry_count, size_t & samples_read) const
{
    int ret = _read(_handle, buffer, buffer_entry_count * sizeof(uint16_t));
    if (ret < 0)
    {
        return false;
    }

    samples_read = ret / sizeof(uint16_t);
    convert_samples_in_place(buffer, samples_read, _adc_center_value);
    return true;
}
