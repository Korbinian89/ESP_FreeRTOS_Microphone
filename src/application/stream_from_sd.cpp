/********************************************************************************************
 * Sixth Application
 * Configure Firebase + LED + ADC + DAC + SD card
 * Upload audio to firebase, download, play
 ********************************************************************************************/
#include "stream_from_sd.h"

// config for i2s
#include "../config/app_config.h"

// config for led
#include "../config/led_config.h"

// contains definitions wifi SSID and PASSWORD
#include "../../include/secrets.h"


void CAppStreamFromSd::setup()
{
  // Sd Card setup
  Serial.print("Setup SD Card\n");
  mSdCard = new CSdCard();
  if (!mSdCard->setup())
  {
    // Early return of setup - if startup fails -> RGB LED won't turn on
    return;
  }


  // launch WiFi
  Serial.printf("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.println("Started up");
  Serial.println(WiFi.localIP());

  // Rgb LED
  Serial.print("Setup RGB LED\n");
  mRgbLed = new CRgbLed();
  mRgbLed->setup();

  // Fb Client and pass interface to firebase client, dependency injection
  Serial.print("Setup FB Client\n");
  mFbClient = new CFbClient();
  mFbClient->setup(mRgbLed, mSdCard);

  //mFbClient->test();

#if 0
  // NO ADC required
  // setup ADC
  Serial.print("Setup ADC\n");
  mI2sAdcSampler = new AdcSampler(ADC_UNIT_1, ADC1_CHANNEL_7, I2S_NUM_0, i2SConfigAdc);
  
  // create task - second core
  Serial.print("Setup ADC Task\n");
  xTaskCreatePinnedToCore(CAppStreamFromSd::i2s_read_and_send_task, "ADC Read and WiFi Write Task", 16384, this, 1, &mI2sReadTaskHandle, 1);
#endif 
  // DAC
  Serial.print("Setup DAC\n");
  mI2sDacSampler = new DacSampler(i2SPinsDac, I2S_NUM_1, i2SConfigDac);

  // create task - first core
  Serial.print("Setup DAC Task\n");
  xTaskCreatePinnedToCore(CAppStreamFromSd::i2s_recv_and_write_task, "DAC Write and WiFi Read Task", 16384, this, 1, &mI2sWriteTaskHandle, 1);

  // setup push button
  setup_push_button();
  Serial.print("Setup Push Button\n");

  Serial.print("Setup - done\n");
}


#if 0

void CAppStreamFromSd::i2s_read_and_send_task(void *param)
{ 
  auto        pThis     = static_cast<CAppStreamFromSd*>(param);
  I2sSampler* pSampler  = pThis->mI2sAdcSampler;
  CFbClient*  pFbClient = pThis->mFbClient;
  CSdCard*    pSdCard   = pThis->mSdCard;
  int16_t*    pSamples  = (int16_t *)malloc(sizeof(int16_t) * 1024 * 1); // store 1 dma buffer

  if (!pSamples)
  {
    Serial.println("Failed to allocate memory for samples");
    return;
  }
  else
  {
    Serial.println("Succeeded to allocate memory for samples");
  }

  while (true)
  { 
    // suspend this until button gets pressed
    vTaskSuspend(NULL);

    Serial.println("adcReadTask: resume ADC and start I2S");

    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("Stack Consumption: %u\n", stackHighWaterMark);

    if (pSampler)
    {
      Serial.println("ADC Sampler valid - start");
      pSampler->start();
    }
    else
    {
      Serial.println("ADC Sampler invalid - error");
    }

    int  samplesTotal = 0;
    int  idx = 0;
    auto startTime = micros();

    // open file - write 
    pSdCard->open(false);

    while ( samplesTotal <=  NUM_OF_SAMPLES_PER_SECOND / 32 /*seconds*/) 
    {
      if ( Firebase.ready())
      {
        int samplesRead = pSampler->read(pSamples, 1024 * 1 /* one dma buffer at once */);
        samplesTotal += samplesRead;
        // SD write
        int bytesWrite = pSdCard->write((uint8_t*)pSamples, samplesRead * sizeof(int16_t), idx++);
      }
    }
    
    auto endTime = micros();
    Serial.printf("Capture: %lu\n", (endTime - startTime));
    
    // close file
    pSdCard->close();

    // stop sampler
    pSampler->stop();

    // upload file
    pFbClient->upload_audio();

    Serial.println("adcReadTask: finished");

    // wait 3s and then start receiving mode
    vTaskDelay(3000);

    // Resume speaker task
    vTaskResume(pThis->mI2sWriteTaskHandle);
  }
}
#endif

void CAppStreamFromSd::i2s_recv_and_write_task(void *param)
{
  size_t      bufSize   = 1024;
  auto        pThis     = static_cast<CAppStreamFromSd*>(param);
  I2sSampler* pSampler  = pThis->mI2sDacSampler;
  CFbClient*  pFbClient = pThis->mFbClient;
  CSdCard*    pSdCard   = pThis->mSdCard;
  int16_t*    pSamples  = (int16_t *)malloc(sizeof(int16_t) * bufSize); // 1 DMA buffer with 1024 samples each

  while (true)
  {
    vTaskSuspend(NULL);

    Serial.println("dacWriteTask: resume DAC and start I2S");
    
    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("Stack Consumption: %u\n", stackHighWaterMark);

    if (pSampler)
    {
      Serial.println("DAC Sampler valid - start");
      // start I2S DAC
      pSampler->start();
    }
    else
    {
      Serial.println("DAC Sampler invalid - error");
    }

    while(!Firebase.ready())
    {
      Serial.println("Waiting for Firebase");
    }

    // delete old downlaod
    pSdCard->delete_recording_download("/sample_16kHz_signed_16bit_download.raw");

    // upload audio
    pFbClient->upload_audio_to_firebase_storage();
    pFbClient->download_audio_from_firebase_storage();

    int  totalSamples = 0;
    int bytesRead = 0;
    int  idx = 0;
    auto startTime = micros();

    // open file - read 
    pSdCard->open(true, "/sample_16kHz_signed_16bit_download.raw");

    // 16 kHz
    do
    {
      bytesRead = pSdCard->read((uint8_t*)pSamples, bufSize * sizeof(int16_t), idx++);

      if (bytesRead > 0)
      {
        int samplesWritten = pSampler->write(pSamples, bufSize);
        totalSamples += samplesWritten;
      }
    }
    while(bytesRead > 0);

    auto endTime = micros();
    Serial.printf("Play: %lu\n", (endTime - startTime));

    // close file
    pSdCard->close();

    // stop client and DAC
    pSampler->stop();

    Serial.println("dacWriteTask: finished");
  }
}


/**********************************************************************
 * Resume task when button is pressed
 **********************************************************************/
void IRAM_ATTR CAppStreamFromSd::button_resume_task(void *param)
{
  auto pThis = static_cast<CAppStreamFromSd*>(param);

  xTaskResumeFromISR(pThis->mI2sWriteTaskHandle);
}

/**********************************************************************
 * Setup push button
 **********************************************************************/
void CAppStreamFromSd::setup_push_button()
{
  gpio_pad_select_gpio(PUSH_BUTTON_PIN);

  gpio_set_direction(PUSH_BUTTON_PIN, GPIO_MODE_INPUT);

  gpio_set_intr_type(PUSH_BUTTON_PIN, GPIO_INTR_POSEDGE);

  gpio_install_isr_service(ESP_INR_FLAG_DEFAULT);

  gpio_isr_handler_add(PUSH_BUTTON_PIN, CAppStreamFromSd::button_resume_task, this);
}