#include "wav_file.h"


WavFile::WavFile(const std::string & file_name, size_t samples_per_second) :
    _name(file_name), _samples_per_second(samples_per_second)
{
}

WavFile::~WavFile()
{
}

const std::string & WavFile::name() const
{
    return _name;
}

bool WavFile::create()
{
    return false;
}

bool WavFile::write_samples(const int16_t * sample_buffer, size_t number_of_samples)
{
    return false;
}
