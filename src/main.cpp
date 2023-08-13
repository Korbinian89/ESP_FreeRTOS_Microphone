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
I2sSampler * i2sSampler;

// dac sampler
DacSampler * dacSampler;

// circular buffer
uint8_t * circularBuffer;


// timer event
hw_timer_t * timer = NULL; 

// dac read from wifi client
WiFiClient dacWifiClient;

// timer mux
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; 

TaskHandle_t dacWriteTaskHandle;
TaskHandle_t i2sReadTaskHandle;

// Consts
const int      SAMPLE_RATE                = 16000;
const int      NUM_OF_SAMPLES_PER_SECOND  = SAMPLE_RATE;
const uint16_t ADC_SERVER_PORT            = 12345;
const int      DAC_BUFFER_MAX_SAMPLES     = 8192;

// globals 
int            dacBufferReadPointer       = 0;
int            dacBufferWritePointer      = 1;
bool           dacOutputPlay              = false;

// config
i2s_config_t i2SConfigAdc =
{
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
  .sample_rate = SAMPLE_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_MSB),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 4,
  .dma_buf_len = 1024,
  .use_apll = false,
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0
};

i2s_config_t i2SConfigDac =
{
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
  .sample_rate = SAMPLE_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_8BIT,
  .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
  .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_MSB),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 4,
  .dma_buf_len = 1024,
  .use_apll = false,
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0,
  .bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT
};





/**********************************************************************
 * DAC circular buffer
 **********************************************************************/
int getDistance()
{
  int distance = 0;
  if (dacBufferReadPointer < dacBufferWritePointer ) distance =  DAC_BUFFER_MAX_SAMPLES - dacBufferWritePointer + dacBufferReadPointer;
  else if (dacBufferReadPointer > dacBufferWritePointer ) distance = dacBufferReadPointer - dacBufferWritePointer;
  return distance;
}


/**********************************************************************
 * Setup dac and timer
 **********************************************************************/
void IRAM_ATTR onTimer()
{
  portENTER_CRITICAL_ISR(&timerMux);
  
  // play data: 
  if (dacOutputPlay)
  {
    dac_output_voltage(DAC_CHANNEL_1, circularBuffer[dacBufferReadPointer]);

    dacBufferReadPointer++;
    if (dacBufferReadPointer == DAC_BUFFER_MAX_SAMPLES)
    {
      dacBufferReadPointer = 0;
    }

    if ( getDistance() == 0 )
    {
      Serial.println("Buffer underrun!!!");
      dacOutputPlay = false;
    }
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

/**********************************************************************
 * Setup dac and timer
 **********************************************************************/
void setup_dac_and_timer()
{
  dac_output_enable(DAC_CHANNEL_1);

  pinMode(33, INPUT_PULLUP);
  pinMode(32, INPUT_PULLUP);

  // Timer 0 has 80MHz -> Presale of 2 -> 40MHz tick
  timer = timerBegin(0, 2, true);
  timerAttachInterrupt(timer, &onTimer, true);

  // Interrupt every 2500 ticks -> 16kHz -> Samplerate of test_UnSigned8bit_PCM.raw
  timerAlarmWrite(timer, 2500, true);
  timerAlarmEnable(timer);
}

/**********************************************************************
 * Stop dac and timer
 **********************************************************************/
void stop_dac_and_timer()
{
  dac_output_disable(DAC_CHANNEL_1);
  timerAlarmDisable(timer);
  timerDetachInterrupt(timer);
  timerStop(timer);
}

/**********************************************************************
 * Task to write samples from ADC to our server
 **********************************************************************/
void i2sReadAndSendTask(void *param)
{
  // generic sampler
  I2sSampler* pSampler = (I2sSampler*) param;
  int16_t*    pSamples = (int16_t *)malloc(sizeof(int16_t) * NUM_OF_SAMPLES_PER_SECOND);

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
      int samplesRead = pSampler->read(pSamples, NUM_OF_SAMPLES_PER_SECOND);
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
  //DacSampler* pSampler = (DacSampler*) param;
  
  while (true)
  { 
    vTaskSuspend(NULL);

    Serial.println("dacWriteTask: resume DAC and start I2S");

    // start I2S DAC
    //pSampler->start();

    // start filling circular buffer
    setup_dac_and_timer();

    // reset buffer cache
    dacBufferReadPointer  = 0;
    dacBufferWritePointer = 1;

    do
    {
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

      int distance = getDistance();
      if (distance <= 1024) dacOutputPlay = true;

      // get 1024 new samples
      if (distance >= 1024) 
      {
        dacWifiClient.write( B11111111 ); // send the command to send new data
    
        // read new data: 
        while (dacWifiClient.available() == 0);
        while (dacWifiClient.available() >= 1) 
        {
          uint8_t value = dacWifiClient.read();
          circularBuffer[dacBufferWritePointer] = value;
          dacBufferWritePointer++;
          if (dacBufferWritePointer == DAC_BUFFER_MAX_SAMPLES) dacBufferWritePointer = 0;
        }
      }
    }
    // connection to dac client still valid
    while( dacWifiClient );

    Serial.println("dacWriteTask: client disconnected - stop dac");
    
    delay(100);

    stop_dac_and_timer();

    // stop client and DAC
    //pSampler->stop();

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
#ifdef SUPPORT_ADC_I2S_SAMPLER
  i2sSampler = new AdcSampler(ADC_UNIT_1, ADC1_CHANNEL_6, I2S_NUM_0, i2SConfigAdc);
#endif
  
  // create task - second core
  Serial.print("Setup Adc Task\n");
  xTaskCreatePinnedToCore(i2sReadAndSendTask, "ADC Read and WiFi Write Task", 4096, i2sSampler, 1, &i2sReadTaskHandle, 1);

  // DAC
#if 0
  Serial.print("Setup DAC\n");
  dacSampler = new DacSampler(I2S_DAC_CHANNEL_BOTH_EN, I2S_NUM_0, i2SConfigDac);
#endif

  // create task - first core
  Serial.print("Setup Adc Task\n");
  xTaskCreatePinnedToCore(dacWriteTask, "DAC Play from circ buf", 4096, nullptr /*dacSampler*/, 1, &dacWriteTaskHandle, 0);

  // create circular buffer for DAC - we get 
  circularBuffer = (uint8_t*)malloc(DAC_BUFFER_MAX_SAMPLES);

  Serial.print("Setup - done\n");

}


/**********************************************************************
 * Loopy Loop
 **********************************************************************/
void loop() 
{
}