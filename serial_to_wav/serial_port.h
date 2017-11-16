#pragma once

#include "i_serial_port.h"

#include <string>
#include <vector>
#include <mutex>


extern uint8_t expected_simulation_start_sequence[9];

class SerialPort : public iSerialPort
{
public:
    SerialPort(const std::string & port_name, bool port_simulated_from_file = false);
    ~SerialPort();
    const std::string & name() const;
    virtual bool open();
    // Non-blocking call to read all available bytes
    virtual bool read_bytes(uint8_t *buffer, size_t buffer_byte_count, size_t & bytes_read) const;
    // Non-blocking call to read all available samples
    virtual bool read_samples(uint16_t * buffer, size_t buffer_entry_count, size_t & samples_read) const;
protected:
    static void read_from_port_thread_launch(SerialPort * _this);
    void read_from_port_thread();
    void stop_reader_thread();

    void store_bytes(const uint8_t *data, size_t bytes);

    void convert_samples_in_place(uint16_t * buffer, size_t num_samples) const;
private:
    std::string _name;
    bool _port_simulated_from_file;
    void *_handle;
    bool _stop_reader_thread;
    void *_thread_handle;
    mutable std::mutex _data_buffer_lock;
    mutable std::vector<uint8_t> _data_buffer;
    mutable size_t _total_bytes_stored;
    mutable size_t _total_bytes_read;
};
