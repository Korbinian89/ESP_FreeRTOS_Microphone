#include <Arduino.h>
#include <FreeRTOSConfig.h>
#include <thread>
#include <freertos/FreeRTOS.h>

#include <driver/adc.h>
#include <driver/i2s.h>

#include <WiFi.h>
#include <HTTPClient.h>

#include <esp_log.h>

// Insert your network credentials
#define WIFI_SSID "FRITZ!Box 7530 DD"
#define WIFI_PASSWORD "65684621901675575713"



WiFiClient * wifiClient;
HTTPClient * httpClient;
WiFiServer * wifiServer;

// Consts
const int sampleRate = 40000;
const int SAMPLE_SIZE = 2048;

//const char*    ADC_SERVER_IP   = "192.168.178.25";
//IPAddress      ADC_SERVER_IP = (192,168,178,25);
const uint16_t ADC_SERVER_PORT = 12345;

// config WTF?
i2s_config_t adcI2SConfig =
{
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
  .sample_rate = 40000,
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



void AdcSamplerEnableI2s()
{
  // locks the ADC
  i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_6);
  i2s_adc_enable(I2S_NUM_0);
}

void AdcSamplerDisableI2s()
{
  // frees the ADC
  i2s_adc_disable(I2S_NUM_0);
}

void AdcSamplerStart()
{
  // install driver
  i2s_driver_install(I2S_NUM_0, &adcI2SConfig, 0, NULL);

  // enable and lock ADC
  AdcSamplerEnableI2s();
}

void AdcSamplerStop()
{
  // revers order
  AdcSamplerDisableI2s();

  // uninstall driver
  i2s_driver_uninstall(I2S_NUM_0);
}

// read 32kB ( 16384 * 2Byte )
int AdcReadI2s(int16_t* samples, int count)
{
  // read from i2s
  size_t bytes_read = 0;
  i2s_read(I2S_NUM_0, samples, sizeof(int16_t) * count, &bytes_read, portMAX_DELAY);
  int samples_read = bytes_read / sizeof(int16_t);
  for (int i = 0; i < samples_read; i++)
  {
    samples[i] = (2048 - (uint16_t(samples[i]) & 0xfff)) * 15;

  }
  return samples_read;
}


void sendData(WiFiClient& iWifiClient, uint8_t* iData, size_t iCount)
{
  iWifiClient.write(iData, iCount);
}


// Task to write samples from ADC to our server
void adcReadAndSendTask(void *param)
{

  // Pass function pointer and convert
  int(*functionPtr)(int16_t*, int) = (int(*)(int16_t*, int))param;

  int16_t *pSamples = (int16_t *)malloc(sizeof(uint16_t) * SAMPLE_SIZE);
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
    
    do
    {
      Serial.println("Waiting for client");
      delay(1000);
      myClient = wifiServer->available();
    }
    while( !myClient );

    if ( myClient ) 
    {
      Serial.println("Client accepted");
    }
    else
    {
      break;
    }

    // call passed function and sleep
    int samplesRead = functionPtr(pSamples, SAMPLE_SIZE);
    Serial.println("Returned: Samples read: " + String(samplesRead) + " - First samples: " + String(pSamples[0]) + "\n");
    size_t bytesSent = myClient.write((uint8_t*) pSamples, ( samplesRead * sizeof(int16_t) ) );
    Serial.println("Bytes sent: " + String(bytesSent));

    myClient.stop();

    delay(1000);
  }
}



void setup() {
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
  AdcSamplerStart();

  // create task
  Serial.print("Setup Task\n");
  TaskHandle_t adcWriterTaskHandle;
  xTaskCreatePinnedToCore(adcReadAndSendTask, "ADC Writer Task", 4096, (void*)AdcReadI2s, 1, &adcWriterTaskHandle, 1);

  Serial.print("Setup - done\n");

}

void loop() {
  // this won't work work with client code and python server
  // no problem with server on ESP

#if 0

  // put your main code here, to run repeatedly:
  // connect wifi client if not connected
  if ( WiFi.status() != WL_CONNECTED )
  {
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
  }
  Serial.println("Waiting for Client");

  WiFiClient myClient;

  do
  {
    delay(1000);
    myClient = wifiServer->available();
  }
  while( !myClient );
  
  // operator overload with connected
  if ( myClient )
  {
    Serial.println("Client accepted and connected ");

    while ( myClient.connected() )
    {
      Serial.println("Client still connected");
      size_t sendBytes = myClient.write("HI");
      Serial.println("Bytes sent: " + String(sendBytes));
      myClient.stop();
    }
  }
  else
  {
    Serial.println("Invalid Client");
  }

  if ( !wifiClient->connected() )
  {
    int rtrn = wifiClient->connect(ADC_SERVER_IP, ADC_SERVER_PORT);
    Serial.println("Rtrn Value: " + String(rtrn));
    
    
    if ( wifiClient->connect(ADC_SERVER_IP, ADC_SERVER_PORT) )
    {
      Serial.println("Wifi Client connected to server successfully");
      wifiClient->write("HI");
      wifiClient->stop();
    }
    else
    {
      Serial.println("Wifi Client connection to server failed - try again");
    }
  }
#endif
  delay(1000);

}