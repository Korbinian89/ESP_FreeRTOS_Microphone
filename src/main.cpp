#include <Arduino.h>
#include <FreeRTOSConfig.h>
#include <thread>
#include <freertos/FreeRTOS.h>

#include <driver/adc.h>
#include <driver/i2s.h>

#include <WiFi.h>

#include <esp_log.h>

// contains definitions wifi SSID and PASSWORD
#include "../include/secrets.h"

// get sampler
#include "sampler/adc_sampler.h"

// definition which class shall be used
#define SUPPORT_ADC_I2S_SAMPLER

// wifi server - provides samples
WiFiServer * wifiServer;

// base class of samplers
I2sSampler * i2sSampler;

// Consts
const int      NUM_OF_SAMPLES  = 16384;
const uint16_t ADC_SERVER_PORT = 12345;

// config
i2s_config_t i2SConfig =
{
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
  //.sample_rate = 40000,
  .sample_rate = 20000,
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
 * Task to write samples from ADC to our server
 **********************************************************************/
void i2sReadAndSendTask(void *param)
{
#if 0
  // Pass function pointer and convert
  // int read(int16_t*, int)
  int(*functionPtr)(int16_t*, int) = (int(*)(int16_t*, int))param;
#endif

  // generic sampler
  I2sSampler* pSampler = (I2sSampler*) param;
  int16_t*    pSamples = (int16_t *)malloc(sizeof(uint16_t) * NUM_OF_SAMPLES);
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
    WiFiClient myClient;
    
    // wait for connection
    do
    {
      Serial.println("Waiting for client");
      delay(1000);
      myClient = wifiServer->available();
    }
    while( !myClient );

    Serial.println("Client accepted");

    while ( myClient ) 
    {
      // send as long as you want until client breaks up
      int samplesRead = pSampler->read(pSamples, NUM_OF_SAMPLES);
      Serial.println("Returned: Samples read: " + String(samplesRead) + " - First samples: " + String(pSamples[0]) + "\n");
      size_t bytesSent = myClient.write((uint8_t*) pSamples, ( samplesRead * sizeof(int16_t) ) );
      Serial.println("Bytes sent: " + String(bytesSent));
    }
    myClient.stop();
    delay(1000);
  }
}


/**********************************************************************
 * Setup system
 **********************************************************************/
void setup() 
{
  Serial.begin(115200);

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


  // setup network server
  wifiServer = new WiFiServer(ADC_SERVER_PORT, 1 /* one client */);
  wifiServer->begin();

  // setup ADC
  Serial.print("Setup ADC\n");

  // 
#ifdef SUPPORT_ADC_I2S_SAMPLER
  i2sSampler = new AdcSampler(ADC_UNIT_1, ADC1_CHANNEL_6, I2S_NUM_0, i2SConfig);
#endif

  i2sSampler->start();
  
  // create task
  Serial.print("Setup Task\n");
  TaskHandle_t i2sWriterTaskHandle;
  xTaskCreatePinnedToCore(i2sReadAndSendTask, "ADC Read and WiFi Write Task", 4096, i2sSampler, 1, &i2sWriterTaskHandle, 1);

  Serial.print("Setup - done\n");

}


/**********************************************************************
 * Loopy Loop
 **********************************************************************/
void loop() 
{
  // nothing to be done
  delay(1000);
}