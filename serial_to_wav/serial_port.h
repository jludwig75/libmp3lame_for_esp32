#pragma once

#include "i_serial_port.h"

#include <string>


class SerialPort : public iSerialPort
{
public:
    SerialPort(const std::string & port_name);
    ~SerialPort();
    const std::string & name() const;
    virtual bool open();
    // Non-blocking call to read all available samples
    virtual bool read_samples(uint16_t * buffer, size_t buffer_entry_count, size_t & samples_read) const;
private:
    std::string _name;
};
