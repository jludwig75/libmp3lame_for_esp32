#pragma once

#include <stdint.h>
#include <string>


struct wav_file;


class WavFile
{
public:
    WavFile(const std::string & file_name, size_t samples_per_second);
    ~WavFile();
    const std::string & name() const;
    bool create();
    void close();
    bool write_samples(const int16_t * sample_buffer, size_t number_of_samples);
private:
    std::string _name;
    size_t _samples_per_second;
    struct wav_file *_wav_header;
    int _file_handle;
    size_t _bytes_per_sample;
};
