#include "data_converter.h"

#include "utils.h"

#include <assert.h>



DataConverter::DataConverter(uint16_t center_value, uint16_t adc_max) :
    _center_value(center_value), _adc_max(adc_max)
{
    assert(2 * _center_value < _adc_max);
}

int16_t DataConverter::convert_adc_level_to_sample(uint16_t adc_level) const
{
    assert(adc_level < _adc_max);

    if (adc_level > 2 * _center_value)
    {
        adc_level = 2 * _center_value;
    }

    return map<int16_t>(adc_level, 0, 2 * _center_value, INT16_MIN, INT16_MAX);

}

void DataConverter::convert_adc_levels_to_samples(const uint16_t *adc_levels, int16_t *audio_samples, size_t samples_to_convert) const
{
    for (unsigned i = 0; i < samples_to_convert; i++)
    {
        audio_samples[i] = adc_levels[i];
    }
}
