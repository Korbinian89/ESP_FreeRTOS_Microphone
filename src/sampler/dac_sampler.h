/**********************************************************************
 * ADC Derived Sampler Class
 **********************************************************************/
#pragma once

#include <driver/dac.h>
#include <driver/i2s.h>


class DacSampler
{
public:
    DacSampler(i2s_pin_config_t iI2sPinConfig, i2s_port_t iI2sPort, const i2s_config_t& iI2sConfig);
    ~DacSampler();

    size_t write(int16_t* samples, int count) ;
    void start();
    void stop();

private:

    uint16_t process_sample(uint16_t sample)
    {
        // DAC needs unsigned 16 bit samples
        return sample + 32768;
    }

    i2s_port_t       mI2sPort      {};
    i2s_config_t     mI2sConfig    {};
    i2s_pin_config_t mI2sPinConfig {};
    QueueHandle_t    mI2sQueue     {};
    int16_t         *mFrames;
};