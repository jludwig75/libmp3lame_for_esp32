#pragma once

#include <stdint.h>


class DataAnalyzer
{
public:
    DataAnalyzer();
    void record_adc_samples(uint16_t *adc_levels, size_t num_samples);
    uint16_t avarage() const;
    uint16_t min() const;
    uint16_t max() const;

private:
    uint64_t _sum;
    size_t _count;
    uint16_t _min;
    uint16_t _max;
};