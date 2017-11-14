#pragma once

#include <stdint.h>


class DataConverter
{
public:
    DataConverter(uint16_t center_value, uint16_t adc_max);
    int16_t convert_adc_level_to_sample(uint16_t adc_level) const;
    void convert_adc_levels_to_samples(const uint16_t *adc_levels, int16_t *audio_samples, size_t samples_to_convert) const;
private:
    uint16_t _center_value;
    uint16_t _adc_max;
};
