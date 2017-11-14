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

bool SerialPort::read_samples(uint16_t * buffer, size_t buffer_entry_count, size_t & samples_read) const
{
    return 0;
}
