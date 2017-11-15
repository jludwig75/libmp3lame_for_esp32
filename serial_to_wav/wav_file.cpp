#include "wav_file.h"

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

#define FIELD_OFFSET(type, field)  ((size_t)&(((type *)0)->field))

#pragma warning(push)
#pragma warning(disable : 4200)
#pragma pack(push, 1)
struct riff_header
{
    char chunk_id[4];
    uint32_t chunk_size;
    char format[4];
};

struct fmt_sub_chunk
{
    char sub_chunk_id[4];
    uint32_t sub_chunk_size;
    uint16_t audio_format;
    uint16_t number_of_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct data_sub_chunk
{
    char sub_chunk_id[4];
    uint32_t sub_chunk_size;
    uint8_t data[0];
};

struct wav_file
{
    riff_header header;
    fmt_sub_chunk format;
    data_sub_chunk data;
};
#pragma pack(pop)
#pragma warning(pop)


WavFile::WavFile(const std::string & file_name, size_t samples_per_second) :
_name(file_name), _samples_per_second(samples_per_second), _wav_header(NULL), _file_handle(-1), _bytes_per_sample(0)
{
    _wav_header = new wav_file;

    memset(_wav_header, 0, sizeof(*_wav_header));

    // Initialize the RIF header
    memcpy(_wav_header->header.chunk_id, "RIFF", sizeof(_wav_header->header.chunk_id));
    _wav_header->header.chunk_size = sizeof(_wav_header->header) - FIELD_OFFSET(riff_header, format) + sizeof(_wav_header->format) + sizeof(_wav_header->data);
    memcpy(_wav_header->header.format, "WAVE", sizeof(_wav_header->header.format));

    // Initialize the format sub chunk
    memcpy(_wav_header->format.sub_chunk_id, "fmt ", sizeof(_wav_header->format.sub_chunk_id));
    _wav_header->format.sub_chunk_size = sizeof(_wav_header->format) - FIELD_OFFSET(fmt_sub_chunk, audio_format);
    _wav_header->format.audio_format = 1;   // PCM
    _wav_header->format.number_of_channels = 1;
    _wav_header->format.sample_rate = 8000; // 8 kHz
    _wav_header->format.bits_per_sample = 16;
    _wav_header->format.block_align = _wav_header->format.number_of_channels * _wav_header->format.bits_per_sample / sizeof(uint8_t);
    _wav_header->format.byte_rate = _wav_header->format.sample_rate * _wav_header->format.number_of_channels * _wav_header->format.bits_per_sample / sizeof(uint8_t);

    // Initialize the data sub chunk
    memcpy(_wav_header->data.sub_chunk_id, "data", sizeof(_wav_header->data.sub_chunk_id));
    _wav_header->data.sub_chunk_size = 0;

    _bytes_per_sample = _wav_header->format.bits_per_sample / 8;
}

WavFile::~WavFile()
{
    if (_file_handle != -1)
    {
        close();
    }
    delete _wav_header;
    _wav_header = NULL;
}

void WavFile::close()
{
    assert(_file_handle != -1);

    if (_file_handle != -1)
    {
        // Update the header
        _wav_header->header.chunk_size += _wav_header->data.sub_chunk_size;
        _lseek(_file_handle, 0, 0);
        _write(_file_handle, _wav_header, sizeof(*_wav_header));

        // Close the file
        _close(_file_handle);
        _file_handle = -1;
    }
}


const std::string & WavFile::name() const
{
    return _name;
}

bool WavFile::create()
{
    assert(_file_handle == -1);

    errno_t err = _sopen_s(&_file_handle, _name.c_str(), O_BINARY | O_CREAT | O_WRONLY, _SH_DENYRW, _S_IREAD | _S_IWRITE);
    if (err != 0)
    {
        return false;
    }

    int ret = _write(_file_handle, _wav_header, sizeof(*_wav_header));
    if (ret != sizeof(*_wav_header))
    {
        return false;
    }

    return true;
}

bool WavFile::write_samples(const int16_t * sample_buffer, size_t number_of_samples)
{
    if (_file_handle == -1 || !_wav_header)
    {
        return false;
    }

    int ret = _write(_file_handle, sample_buffer, number_of_samples * _bytes_per_sample);
    if (ret > 0)
    {
        _wav_header->data.sub_chunk_size += ret;
    }
    return ret == number_of_samples * _bytes_per_sample;
}
