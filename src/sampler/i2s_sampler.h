/**********************************************************************
 * I2S Sampler Base Class
 **********************************************************************/
#pragma once

#include <driver/i2s.h>

class I2sSampler
{
public:
    I2sSampler(i2s_port_t iI2sPort, const i2s_config_t& iI2sConfig);
    virtual ~I2sSampler() = default;

    // de-/install driver and enable i2s in derived
    void start();
    void stop();
    virtual int read(int16_t* samples, int count) = 0;
    virtual size_t write(int16_t* samples, int count) = 0;


protected:
    virtual void enable_i2s() = 0;
    virtual void disable_i2s() = 0;

    i2s_port_t   mI2sPort   {I2S_NUM_0};
    i2s_config_t mI2sConfig {};
    
};