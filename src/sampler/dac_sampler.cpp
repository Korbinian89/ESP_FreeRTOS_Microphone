/**********************************************************************
 * ADC Derived Sampler Class
 **********************************************************************/
#include <esp_log.h>
#include <HardwareSerial.h>
#include "dac_sampler.h"

const int NUM_OF_FRAMES_TO_SEND = 256;

DacSampler::DacSampler(i2s_dac_mode_t iDacMode, i2s_port_t iI2sPort, const i2s_config_t& iI2sConfig)
  : mDacMode(iDacMode)
  , mI2sPort(iI2sPort)
  , mI2sConfig(iI2sConfig)
{
    mFrames = (uint8_t *)malloc(2 * sizeof(uint8_t) * NUM_OF_FRAMES_TO_SEND);
}

DacSampler::~DacSampler()
{
  free(mFrames);
}


void DacSampler::start()
{
  Serial.printf("Start DAC\n");
  esp_err_t rtrn = i2s_driver_install(mI2sPort, &mI2sConfig, 4, mI2sQueue);

  if ( rtrn == ESP_ERR_INVALID_ARG )
    Serial.printf("Parameter Error\n");
  else if ( rtrn == ESP_ERR_NO_MEM )
    Serial.printf("DAC Out of memory\n");
  else if ( rtrn == ESP_ERR_INVALID_STATE )
    Serial.printf("DAC Current I2S port is in use\n");

  rtrn = i2s_set_dac_mode(mDacMode);

  if ( rtrn == ESP_ERR_INVALID_ARG )
    Serial.printf("DAC Parameter error\n");

  i2s_zero_dma_buffer(mI2sPort);

  i2s_start(mI2sPort);
}

void DacSampler::stop()
{
  i2s_stop(mI2sPort);
  i2s_driver_uninstall(mI2sPort);
}

// sample count
size_t DacSampler::write(uint8_t* iSamples, int iCount)
{
  int sample_index = 0;
  while (sample_index < iCount)
  {
    int samples_to_send = 0;
    for (int i = 0; i < NUM_OF_FRAMES_TO_SEND && sample_index < iCount; i++)
    {
      uint8_t sample = iSamples[sample_index];

      mFrames[i * 2] = sample;  // left channel
      mFrames[i * 2 + 1] = sample;  // right channel
      samples_to_send++;
      sample_index++;
    }
    // write data to the i2s peripheral
    size_t bytes_written = 0;
    i2s_write(mI2sPort, mFrames, samples_to_send * sizeof(uint8_t) * 2, &bytes_written, portMAX_DELAY);
    if (bytes_written != samples_to_send * sizeof(uint8_t) * 2)
    {
      Serial.printf("Did not write all bytes\n");
    }
    Serial.printf("DAC: Samples send: %d, Bytes written: %d\n", samples_to_send, bytes_written);
  }
  return sample_index;
}