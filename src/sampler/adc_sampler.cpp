/**********************************************************************
 * ADC Derived Sampler Class
 **********************************************************************/
#include "adc_sampler.h"


AdcSampler::AdcSampler(adc_unit_t iAdcUnit, adc1_channel_t iAdcChannel, i2s_port_t iI2sPort, const i2s_config_t& iI2sConfig)
  : I2sSampler(iI2sPort, iI2sConfig)
  , mAdcUnit(iAdcUnit)
  , mAdcChannel(iAdcChannel)
{}

void AdcSampler::enable_i2s()
{
  // locks the ADC
  i2s_set_adc_mode(mAdcUnit, mAdcChannel);
  i2s_adc_enable(mI2sPort);
}

void AdcSampler::disable_i2s()
{
  // frees the ADC
  i2s_adc_disable(mI2sPort);
}

int AdcSampler::read(int16_t* iSamples, int iCount)
{  
  // read from i2s
  size_t bytesRead = 0;
  i2s_read(mI2sPort, iSamples, sizeof(int16_t) * iCount, &bytesRead, portMAX_DELAY);
  int samplesRead = bytesRead / sizeof(int16_t);
  for (int i = 0; i < samplesRead; i++)
  {
    iSamples[i] = (2048 - (uint16_t(iSamples[i]) & 0xfff)) * 15;
  }
  return samplesRead;
}