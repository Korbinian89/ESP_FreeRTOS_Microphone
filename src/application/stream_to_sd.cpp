/********************************************************************************************
 * Fourth Application
 * Configure Firebase + LED + ADC + DAC + SD card
 ********************************************************************************************/
#include "stream_to_sd.h"

// config for i2s
#include "../config/app_config.h"

// config for led
#include "../config/led_config.h"

// contains definitions wifi SSID and PASSWORD
#include "../../include/secrets.h"


void CAppStreamToSd::setup()
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
  mFbClient->setup(mRgbLed);

  // setup ADC
  Serial.print("Setup ADC\n");
  mI2sAdcSampler = new AdcSampler(ADC_UNIT_1, ADC1_CHANNEL_7, I2S_NUM_0, i2SConfigAdc);
  
  // create task - second core
  Serial.print("Setup ADC Task\n");
  xTaskCreatePinnedToCore(CAppStreamToSd::i2s_read_and_send_task, "ADC Read and WiFi Write Task", 16384, this, 1, &mI2sReadTaskHandle, 1);

  // DAC
  Serial.print("Setup DAC\n");
  mI2sDacSampler = new DacSampler(i2SPinsDac, I2S_NUM_1, i2SConfigDac);

  // create task - first core
  Serial.print("Setup DAC Task\n");
  xTaskCreatePinnedToCore(CAppStreamToSd::i2s_recv_and_write_task, "DAC Write and WiFi Read Task", 16384, this, 1, &mI2sWriteTaskHandle, 1);

  // setup push button
  setup_push_button();
  Serial.print("Setup Push Button\n");

  Serial.print("Setup - done\n");
}




void CAppStreamToSd::i2s_read_and_send_task(void *param)
{ 
  auto        pThis     = static_cast<CAppStreamToSd*>(param);
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

    int samplesTotal = 0;
    int idx = 0;

    auto startTime = micros();

    // open file - write 
    pSdCard->open(false);

    while ( samplesTotal <=  NUM_OF_SAMPLES_PER_SECOND * 3 /*seconds*/) 
    {
      if ( Firebase.ready())
      {
        //auto startTimeAdc = micros();

        int samplesRead = pSampler->read(pSamples, 1024 * 1 /* one dma buffer at once */);
        samplesTotal += samplesRead;
        //Serial.printf("Returned: Samples read: %d\n", samplesRead);

        //auto endTimeAdc = micros();
        //auto startTimeFb = micros();

        // SD write
        int bytesWrite = pSdCard->write((uint8_t*)pSamples, samplesRead * sizeof(int16_t), idx++);
        //Serial.printf("Bytes sent: %d\n", bytesWrite);

        //auto endTimeFb = micros();

        //Serial.printf("ADC: %lu\n", (endTimeAdc - startTimeAdc));
        //Serial.printf("FB: %lu\n", (endTimeFb - startTimeFb));
      }
    }
    
    auto endTime = micros();
    Serial.printf("Capture: %lu\n", (endTime - startTime));
    
    // close file
    pSdCard->close();

    pSampler->stop();

    Serial.println("adcReadTask: finished");

    // wait 3s and then start receiving mode
    vTaskDelay(3000);

    // Resume speaker task
    vTaskResume(pThis->mI2sWriteTaskHandle);
  }
}


void CAppStreamToSd::i2s_recv_and_write_task(void *param)
{
  auto        pThis    = static_cast<CAppStreamToSd*>(param);
  I2sSampler* pSampler = pThis->mI2sDacSampler;
  CFbClient*  pFbClient = pThis->mFbClient;
  CSdCard*    pSdCard   = pThis->mSdCard;
  int16_t*    pSamples = (int16_t *)malloc(sizeof(int16_t) * 1024); // 1 DMA buffer with 1024 samples each

  while (true)
  {
    // until download data from FB is needed 
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
      Serial.println("ADC Sampler invvalid - error");
    }
    
    int totalSamples = 0;
    int idx = 0;

    auto startTime = micros();

    // open file - read 
    pSdCard->open(true);

    while(totalSamples < NUM_OF_SAMPLES_PER_SECOND * 3 /*seconds*/)
    {
      //auto startTimeDac = micros();

      int bytesRead = pSdCard->read((uint8_t*)pSamples, 1024 * sizeof(int16_t), idx++);
      //Serial.printf("Bytes received: %d\n", bytesRead);

      //auto endTimeDac = micros();
      //auto startTimeFb = micros();

      int samplesWritten = pSampler->write(pSamples, 1024);
      //Serial.printf("Samples written %d\n", samplesWritten);
      totalSamples += samplesWritten;
  
      //auto endTimeFb = micros();

      //Serial.printf("DAC: %lu\n", (endTimeDac - startTimeDac));
      //Serial.printf("FB: %lu\n", (endTimeFb - startTimeFb));
    }

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
void IRAM_ATTR CAppStreamToSd::button_resume_task(void *param)
{
  auto pThis = static_cast<CAppStreamToSd*>(param);

  xTaskResumeFromISR(pThis->mI2sReadTaskHandle);
}

/**********************************************************************
 * Setup push button
 **********************************************************************/
void CAppStreamToSd::setup_push_button()
{
  gpio_pad_select_gpio(PUSH_BUTTON_PIN);

  gpio_set_direction(PUSH_BUTTON_PIN, GPIO_MODE_INPUT);

  gpio_set_intr_type(PUSH_BUTTON_PIN, GPIO_INTR_POSEDGE);

  gpio_install_isr_service(ESP_INR_FLAG_DEFAULT);

  gpio_isr_handler_add(PUSH_BUTTON_PIN, CAppStreamToSd::button_resume_task, this);
}