/**********************************************************************
 * ADC Derived Sampler Class
 **********************************************************************/
#pragma once

#include "i2s_sampler.h"


class AdcSampler : public I2sSampler
{
public:
    AdcSampler(adc_unit_t iAdcUnit, adc1_channel_t iAdcChannel, i2s_port_t iI2sPort, const i2s_config_t& iI2sConfig);

    int read(int16_t* samples, int count) override;
    inline size_t write(int16_t* samples, int count) override { /* not used here */ return 0; }


protected:
    void enable_i2s() override;
    void disable_i2s() override;

private:
    adc_unit_t     mAdcUnit    {};
    adc1_channel_t mAdcChannel {};
};