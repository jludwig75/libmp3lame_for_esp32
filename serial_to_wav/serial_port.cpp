#include "serial_port.h"


SerialPort::SerialPort(const std::string & port_name) :
    _name(port_name)
{

}

SerialPort::~SerialPort()
{
}

const std::string & SerialPort::name() const
{
    return _name;
}

bool SerialPort::open()
{
    return false;
}

size_t SerialPort::read_samples(uint16_t * buffer, size_t buffer_size) const
{
    return 0;
}
