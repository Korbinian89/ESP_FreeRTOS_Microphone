#include <Arduino.h>
#include <FreeRTOSConfig.h>
#include <thread>
#include <freertos/FreeRTOS.h>

#include <driver/adc.h>
#include <driver/dac.h>
#include <driver/i2s.h>
#include <esp_system.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <esp_log.h>

// contains definitions wifi SSID and PASSWORD
#include "../include/secrets.h"

// get dac sampler
#include "sampler/dac_sampler.h"
// get adc sampler
#include "sampler/adc_sampler.h"


// definition which class shall be used
#define SUPPORT_ADC_I2S_SAMPLER

// wifi server - provides samples
WiFiServer * wifiServer;

// base class of samplers
I2sSampler * i2sAdcSampler;

// dac sampler
I2sSampler * i2sDacSampler;


TaskHandle_t dacWriteTaskHandle;
TaskHandle_t i2sReadTaskHandle;

// Consts
const int      SAMPLE_RATE                = 32000;
const int      NUM_OF_SAMPLES_PER_SECOND  = SAMPLE_RATE;
const uint16_t ADC_SERVER_PORT            = 12345;
const int      DAC_BUFFER_MAX_SAMPLES     = 8192;


// config
i2s_config_t i2SConfigAdc =
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
i2s_config_t i2SConfigDac =
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
i2s_pin_config_t i2SPinsDac = 
{
  .bck_io_num = GPIO_NUM_27,
  .ws_io_num = GPIO_NUM_14,
  .data_out_num = GPIO_NUM_26,
  .data_in_num = -1
};


/**********************************************************************
 * Task to write samples from ADC to our server
 **********************************************************************/
void i2sReadAndSendTask(void *param)
{
  // generic sampler
  I2sSampler* pSampler = (I2sSampler*) param;
  //int16_t*    pSamples = (int16_t *)malloc(sizeof(int16_t) * NUM_OF_SAMPLES_PER_SECOND);
  int16_t*    pSamples = (int16_t *)malloc(sizeof(int16_t) * 1024 * 2); // store 2 dma buffer

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
    
    pSampler->start();

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
      //int samplesRead = pSampler->read(pSamples, NUM_OF_SAMPLES_PER_SECOND);
      int samplesRead = pSampler->read(pSamples, 1024 * 2 /* two dma buffer at once */);
      Serial.println("Returned: Samples read: " + String(samplesRead) + " - First samples: " + String(pSamples[0]) + "\n");
      size_t bytesSent = myClient.write((uint8_t*)pSamples, samplesRead * sizeof(int16_t));
      Serial.println("Bytes sent: " + String(bytesSent));
    }
    myClient.stop();
    pSampler->stop();

    delay(100);

    // Resume speaker task
    vTaskResume(dacWriteTaskHandle);

    // suspend this
    vTaskSuspend(NULL);
  }
}

void dacWriteTask(void *param)
{
  I2sSampler* pSampler = (DacSampler*) param;
  int16_t*    pSamples = (int16_t *)malloc(sizeof(int16_t) * 1024); // 1024 samples to 1024 samples DMA buffer


  while (true)
  { 
    vTaskSuspend(NULL);

    Serial.println("dacWriteTask: resume DAC and start I2S");

    // dac read from wifi client
    WiFiClient dacWifiClient;

    // start I2S DAC
    pSampler->start();

    // read from wifi into circular buffer if enabled
    // switch to TcpAsync Library
    // wait for connection
    if ( !dacWifiClient )
    {
      do
      {
        Serial.println("wifiLoop: waiting for client");
        delay(1000);
        dacWifiClient = wifiServer->available();
      }
      while( !dacWifiClient );
    }
    Serial.println("wifiLoop: client accepted");

    while( dacWifiClient )
    {
      int availableBytes = dacWifiClient.available();
      Serial.printf("Availables bytes: %d\n", availableBytes);

      int bytesReceived = dacWifiClient.read((uint8_t*)pSamples, 1024 * sizeof(int16_t));
      Serial.printf("Bytes received: %d\n", bytesReceived);

      int samplesWritten = pSampler->write(pSamples, 1024);
      Serial.printf("Samples written %d\n", samplesWritten);
    }

    Serial.println("dacWriteTask: client disconnected - stop dac");
    
    delay(100);

    // stop client and DAC
    pSampler->stop();

    // resume mic adc task
    vTaskResume(i2sReadTaskHandle);
  }
}


/**********************************************************************
 * Setup system
 **********************************************************************/
void setup() 
{
  Serial.begin(115200);


  Serial.printf("ESP-IDF Version %d.%d.%d \r\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
  Serial.printf("Ardunio Version %d.%d.%d \r\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);

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
  i2sAdcSampler = new AdcSampler(ADC_UNIT_1, ADC1_CHANNEL_7, I2S_NUM_0, i2SConfigAdc);
  
  // create task - second core
  Serial.print("Setup Adc Task\n");
  xTaskCreatePinnedToCore(i2sReadAndSendTask, "ADC Read and WiFi Write Task", 4096, i2sAdcSampler, 1, &i2sReadTaskHandle, 1);

  // DAC
  Serial.print("Setup DAC\n");
  i2sDacSampler = new DacSampler(i2SPinsDac, I2S_NUM_1, i2SConfigDac);

  // create task - first core
  Serial.print("Setup Adc Task\n");
  xTaskCreatePinnedToCore(dacWriteTask, "DAC Write and WiFi Read Task", 4096, i2sDacSampler, 1, &dacWriteTaskHandle, 0);

  Serial.print("Setup - done\n");
}


/**********************************************************************
 * Loopy Loop
 **********************************************************************/
void loop() 
{
}