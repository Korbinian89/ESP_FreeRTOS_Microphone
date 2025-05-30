#pragma once
#include <map>
#include <string>
#include <driver/i2s.h>


#if 0
// Only use these if no ADC is enabled, otherwise we overwrite our pre-recorded test file
const std::string UPLOAD_FILE_NAME = "/sample_16kHz_signed_16bit.raw";
const std::string DOWNLOAD_FILE_NAME = "/sample_16kHz_signed_16bit_download.raw";
const std::string FS_FILE_NAME = "sample_16kHz_signed_16bit.raw";
#endif


const std::string UPLOAD_FILE_NAME = "/recording.raw";
const std::string DOWNLOAD_FILE_NAME = "/recording_download.raw";
const std::string FS_FILE_NAME = "recording.raw";



/**********************************************************************
 * DEFINES
 **********************************************************************/
/* push button for recording */
#define PUSH_BUTTON_PIN GPIO_NUM_33
#define ESP_INR_FLAG_DEFAULT 0


/**********************************************************************
 * CONSTS
 **********************************************************************/
/* I2S */
const int      SAMPLE_RATE                = 32000;
const int      NUM_OF_SAMPLES_PER_SECOND  = SAMPLE_RATE;
const uint16_t ADC_SERVER_PORT            = 12345;
const int      DAC_BUFFER_MAX_SAMPLES     = 8192;



/**********************************************************************
 * Built in ADC
 **********************************************************************/
static i2s_config_t i2SConfigAdc =
{
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
  .sample_rate = SAMPLE_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = I2S_COMM_FORMAT_I2S_LSB,
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 4,
  .dma_buf_len = 1024,
  .use_apll = false,
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0
};


/**********************************************************************
 * Do not use built-in DAC - broken with current ESP Arduino version
 **********************************************************************/
static i2s_config_t i2SConfigDac =
{
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
  .sample_rate = SAMPLE_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
  .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 4,
  .dma_buf_len = 1024
};

/**********************************************************************
 * External DAC via I2S pin layout
 **********************************************************************/
static i2s_pin_config_t i2SPinsDac = 
{
  .bck_io_num = GPIO_NUM_27,    // serial clock
  .ws_io_num = GPIO_NUM_14,     // left right clock (word select)
  .data_out_num = GPIO_NUM_26,  // speaker serial data (DIN on board)
  .data_in_num = -1
};

