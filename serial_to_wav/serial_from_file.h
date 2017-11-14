#pragma once

#include "i_serial_port.h"

#include <string>


class SerialFromFile : public iSerialPort
{
public:
    SerialFromFile(const std::string & file_name, uint16_t adc_center_value);
    ~SerialFromFile();
    virtual bool open();
    // Non-blocking call to read all available samples
    virtual bool read_samples(uint16_t * buffer, size_t buffer_entry_count, size_t & samples_read) const;
private:
    int _handle;
    std::string _name;
    uint16_t _adc_center_value;
};