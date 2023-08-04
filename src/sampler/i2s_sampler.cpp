/**********************************************************************
 * I2S Sampler Base Class
 **********************************************************************/
#include "i2s_sampler.h"


I2sSampler::I2sSampler(i2s_port_t iI2sPort, const i2s_config_t& iI2sConfig)
  : mI2sPort(iI2sPort)
  , mI2sConfig(iI2sConfig)
{}

void I2sSampler::start()
{
    // install driver
    i2s_driver_install(mI2sPort, &mI2sConfig, 0, NULL);

    // enable in derived class
    enable_i2s();
}

void I2sSampler::stop()
{
    // disable in derived class
    disable_i2s();

    // uninstall driver
    i2s_driver_uninstall(mI2sPort);
}