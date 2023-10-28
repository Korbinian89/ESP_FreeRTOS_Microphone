/********************************************************************************************
 * First Application
 ********************************************************************************************/
#include "stream_to_host.h"

// config for i2s
#include "../config/app_config.h"

// contains definitions wifi SSID and PASSWORD
#include "../../include/secrets.h"


void CAppStreamToHost::setup()
{
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
  mWifiServer = new WiFiServer(ADC_SERVER_PORT, 1 /* one client */);
  mWifiServer->begin();

  // setup ADC
  Serial.print("Setup ADC\n");
  mI2sAdcSampler = new AdcSampler(ADC_UNIT_1, ADC1_CHANNEL_7, I2S_NUM_0, i2SConfigAdc);
  
  // create task - second core
  Serial.print("Setup ADC Task\n");
  xTaskCreatePinnedToCore(CAppStreamToHost::i2s_read_and_send_task, "ADC Read and WiFi Write Task", 4096, this, 1, &mI2sReadTaskHandle, 1);

  // DAC
  Serial.print("Setup DAC\n");
  mI2sDacSampler = new DacSampler(i2SPinsDac, I2S_NUM_1, i2SConfigDac);

  // create task - first core
  Serial.print("Setup DAC Task\n");
  xTaskCreatePinnedToCore(CAppStreamToHost::i2s_recv_and_write_task, "DAC Write and WiFi Read Task", 4096, this, 1, &mI2sWriteTaskHandle, 0);

  Serial.print("Setup - done\n");
}




void CAppStreamToHost::i2s_read_and_send_task(void *param)
{ 
  auto        pThis    = static_cast<CAppStreamToHost*>(param);
  I2sSampler* pSampler = pThis->mI2sAdcSampler;
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
    if (pSampler)
    {
        Serial.println("ADC Sampler valid - start");
        pSampler->start();
    }
    else
    {
        Serial.println("ADC Sampler invalid - error");
    }

    // wait for connection
    do
    {
      Serial.println("Waiting for client");
      delay(1000);
      myClient = pThis->mWifiServer->available();
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
    vTaskResume(pThis->mI2sWriteTaskHandle);

    // suspend this
    vTaskSuspend(NULL);
  }
}


void CAppStreamToHost::i2s_recv_and_write_task(void *param)
{
  auto        pThis    = static_cast<CAppStreamToHost*>(param);
  I2sSampler* pSampler = pThis->mI2sDacSampler;
  int16_t*    pSamples = (int16_t *)malloc(sizeof(int16_t) * 1024); // 1024 samples to 1024 samples DMA buffer

  while (true)
  { 
    vTaskSuspend(NULL);

    Serial.println("dacWriteTask: resume DAC and start I2S");

    // dac read from wifi client
    WiFiClient dacWifiClient;

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
    
    // read from wifi into circular buffer if enabled
    // switch to TcpAsync Library
    // wait for connection
    if ( !dacWifiClient )
    {
      do
      {
        Serial.println("wifiLoop: waiting for client");
        delay(1000);
        dacWifiClient = pThis->mWifiServer->available();
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
    vTaskResume(pThis->mI2sReadTaskHandle);
  }
}