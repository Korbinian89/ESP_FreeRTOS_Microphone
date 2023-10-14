/**********************************************************************
 * ADC Derived Sampler Class
 **********************************************************************/
#include <HardwareSerial.h>
#include "adc_sampler.h"


/**********************************************************************
 * CTOR Adc Sampler
 **********************************************************************/
AdcSampler::AdcSampler(adc_unit_t iAdcUnit, adc1_channel_t iAdcChannel, i2s_port_t iI2sPort, const i2s_config_t& iI2sConfig)
  : I2sSampler(iI2sPort, iI2sConfig)
  , mAdcUnit(iAdcUnit)
  , mAdcChannel(iAdcChannel)
{}

/**********************************************************************
 * Enable i2s for Adc
 **********************************************************************/
void AdcSampler::enable_i2s()
{
  // locks the ADC
  i2s_set_adc_mode(mAdcUnit, mAdcChannel);
  i2s_adc_enable(mI2sPort);
}

/**********************************************************************
 * Disable i2s for Adc
 **********************************************************************/
void AdcSampler::disable_i2s()
{
  // frees the ADC
  i2s_adc_disable(mI2sPort);
}

/**********************************************************************
 * Read i2s
 **********************************************************************/
int AdcSampler::read(int16_t* iSamples, int iCount)
{  
  // read from i2s
  size_t bytesRead = 0;
  auto rtrn = i2s_read(mI2sPort, iSamples, sizeof(int16_t) * iCount, &bytesRead, portMAX_DELAY);

  if ( rtrn == ESP_OK )
    Serial.println("ADC Read OK: " + String(bytesRead));
  else
    Serial.println("ADC Failed - Parameter Error");

  int samplesRead = bytesRead / sizeof(int16_t);
  for (int i = 0; i < samplesRead; i++)
  {
    iSamples[i] = (2048 - (uint16_t(iSamples[i]) & 0xfff)) * 15;
  }
  return samplesRead;
}