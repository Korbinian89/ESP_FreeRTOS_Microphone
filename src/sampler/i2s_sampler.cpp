/**********************************************************************
 * I2S Sampler Base Class
 **********************************************************************/
#include "i2s_sampler.h"
#include <HardwareSerial.h>


/**********************************************************************
 * CTOR I2S
 **********************************************************************/
I2sSampler::I2sSampler(i2s_port_t iI2sPort, const i2s_config_t& iI2sConfig)
  : mI2sPort(iI2sPort)
  , mI2sConfig(iI2sConfig)
{}


/**********************************************************************
 * Start I2S
 **********************************************************************/
void I2sSampler::start()
{
    // install driver
    i2s_driver_install(mI2sPort, &mI2sConfig, 0, NULL);


  esp_err_t rtrn = i2s_driver_install(mI2sPort, &mI2sConfig, 0, NULL);

  if ( rtrn == ESP_ERR_INVALID_ARG )
  {
    Serial.printf("I2S Parameter Error\n");
  }
  else if ( rtrn == ESP_ERR_NO_MEM )
  {
    Serial.printf("I2S Out of memory\n");
  }
  else if ( rtrn == ESP_ERR_INVALID_STATE )
    Serial.printf("Current I2S port is in use\n");


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