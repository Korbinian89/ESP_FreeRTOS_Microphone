/**********************************************************************
 * ADC Derived Sampler Class
 **********************************************************************/
#pragma once

#include <driver/dac.h>
#include "i2s_sampler.h"



class DacSampler : public I2sSampler
{
public:
    DacSampler(i2s_pin_config_t iI2sPinConfig, i2s_port_t iI2sPort, const i2s_config_t& iI2sConfig);
    ~DacSampler();

    size_t write(int16_t* samples, int count) override;
    inline int read(int16_t* samples, int count) override { /* not used here */}

#if 0
    void start();
    void stop();
#endif

protected:
    void enable_i2s() override;
    void disable_i2s() override;

private:

    uint16_t process_sample(uint16_t sample)
    {
        // DAC needs unsigned 16 bit samples
        return sample + 32768;
    }

    i2s_pin_config_t mI2sPinConfig {};
    int16_t         *mFrames = nullptr;

#if 0
    i2s_port_t       mI2sPort      {};
    i2s_config_t     mI2sConfig    {};
    QueueHandle_t    mI2sQueue     {};
#endif
};