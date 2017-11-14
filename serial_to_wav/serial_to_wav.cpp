#include "serial_port.h"
#include "wav_file.h"
#include "data_converter.h"
#include "data_analyzer.h"
#include "serial_from_file.h"

#include <vector>
#include <sstream>
#include <memory>

#include <assert.h>
#include <conio.h>


using namespace std;


static bool is_com_port_name(const std::string &name)
{
    if (name.length() < string("COMX").length())
    {
        return false;
    }

    if (name.substr(0, 3) != "COM")
    {
        return false;
    }

    stringstream ss(name.substr(3));
    unsigned int i;
    ss >> i;

    return ss.eof() && !ss.bad();
}

bool open_serial_port(const string & port_name, shared_ptr<iSerialPort> & port)
{
    if (is_com_port_name(port_name))
    {
        port = make_shared<SerialPort>(port_name);
    }
    else
    {
        port = make_shared<SerialFromFile>(port_name, 1852);
    }

    return port->open();
}

int find_center_value(shared_ptr<iSerialPort> port)
{
    const size_t number_of_samples_per_transaction = 128;
    
    DataAnalyzer analyzer;

    std::vector<uint16_t> adc_samples(number_of_samples_per_transaction, 0);

    printf("Finding signal center value. Press 'q' to quit\n");

    while (true)
    {
        if (_kbhit() && tolower(_getch()) == 'q')
        {
            break;
        }

        size_t samples_read;
        if (!port->read_samples(&adc_samples[0], adc_samples.size(), samples_read))
        {
            printf("Error reading from serial port\n");
            return -1;
        }
        assert(samples_read <= adc_samples.size());
        if (samples_read == 0)
        {
            continue;
        }

        analyzer.record_adc_samples(&adc_samples[0], samples_read);
    }

    printf("Min: %u, Center: %u, Max: %u\n", analyzer.min(), analyzer.avarage(), analyzer.max());
    return 0;
}

int record_from_serial_to_wav_file(shared_ptr<iSerialPort> port, const std::string & wav_file_name, uint16_t adc_center_value)
{
    const uint16_t adx_max = 4095;
    const size_t number_of_samples_per_transaction = 128;
    const size_t samples_per_second = 8000; // 8 kHz

    DataConverter converter(adc_center_value, adx_max);

    WavFile wav(wav_file_name, samples_per_second);
    if (!wav.create())
    {
        printf("Failed to create WAV file %s\n", wav.name().c_str());
        return -1;
    }

    std::vector<uint16_t> adc_samples(number_of_samples_per_transaction, adc_center_value);
    std::vector<int16_t> audio_samples(number_of_samples_per_transaction, 0);

    printf("Recording signal to WAV file \"%s\". Press 'q' to quit\n", wav.name().c_str());

    while (true)
    {
        if (_kbhit() && tolower(_getch()) == 'q')
        {
            break;
        }

        size_t samples_read;
        if (!port->read_samples(&adc_samples[0], adc_samples.size(), samples_read))
        {
            printf("Error reading from serial port\n");
            return -1;
        }
        assert(samples_read <= adc_samples.size());
        if (samples_read == 0)
        {
            continue;
        }

        converter.convert_adc_levels_to_samples(&adc_samples[0], &audio_samples[0], samples_read);

        if (!wav.write_samples(&audio_samples[0], samples_read))
        {
            printf("Error writing samples to WAV file %s\n", wav.name().c_str());
            return -1;
        }
    }

    return 0;
}

enum serial_to_wave_operation
{
    serial_to_wave_operation__find_center,
    serial_to_wave_operation__record_wav_file,
    serial_to_wave_operation__undefined
};

int main(int argc, char *argv[])
{
    serial_to_wave_operation op = serial_to_wave_operation__undefined;
    string port_name;
    string wav_file_name;
    uint16_t adc_center_value = 0;

    if (argc == 2)
    {
        op = serial_to_wave_operation__find_center;
        port_name = argv[1];
    }
    else if (argc == 4)
    {
        op = serial_to_wave_operation__record_wav_file;
        port_name = argv[1];
        wav_file_name = argv[2];
        string adc_center_value_string = argv[3];
        stringstream ss(adc_center_value_string);
        ss >> adc_center_value;
        if (!ss.eof() || ss.bad())
        {
            printf("Invalid integer for ADC center value \"%s\"\n", adc_center_value_string.c_str());
            return -1;
        }
    }
    else
    {
        printf("Incorrect number of arguments. Expected syntax:\n");
        printf("Find center value of ADC samples\n");
        printf("\tserial_to_wav <COM port>\n");
        printf("Record ADC samples from COM port to WAV file using ADC center value\n");
        printf("\tserial_to_wav <COM port> <WAV file name> <ADC center value>\n");
        return -1;
    }

    shared_ptr<iSerialPort> port;
    if (!open_serial_port(port_name, port))
    {
        printf("Failed to open serial port %s\n", port_name.c_str());
        return -1;
    }

    switch (op)
    {
    case serial_to_wave_operation__find_center:
        return find_center_value(port);
    case serial_to_wave_operation__record_wav_file:
        return record_from_serial_to_wav_file(port, wav_file_name, adc_center_value);
    default:
        assert(false);
        return -1;
    }

    return 0;
}