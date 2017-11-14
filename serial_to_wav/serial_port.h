#pragma once

#include <stdint.h>
#include <string>


class SerialPort
{
public:
    SerialPort(const std::string & port_name);
    ~SerialPort();
    const std::string & name() const;
    bool open();
    // Non-blocking call to read all available samples
    size_t read_samples(uint16_t * buffer, size_t buffer_size) const;
private:
    std::string _name;
};
