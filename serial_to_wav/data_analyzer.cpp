#include "data_analyzer.h"

#include <assert.h>


DataAnalyzer::DataAnalyzer() :
    _sum(0), _count(0)
{

}

void DataAnalyzer::record_adc_samples(uint16_t *adc_levels, size_t num_samples)
{
    for (size_t i = 0; i < num_samples; i++)
    {
        _sum += adc_levels[i];
        _count++;
    }
}

uint16_t DataAnalyzer::avarage() const
{
    assert(_count > 0);

    return static_cast<uint16_t>(_sum / _count);
}
