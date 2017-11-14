#pragma once


#include <stdint.h>


class iSerialPort
{
public:
    ~iSerialPort() {}
    virtual bool open() = 0;
    // Non-blocking call to read all available samples
    virtual bool read_samples(uint16_t * buffer, size_t buffer_entry_count, size_t & samples_read) const = 0;
};
