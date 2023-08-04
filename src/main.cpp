#include <Arduino.h>
#include <FreeRTOSConfig.h>
#include <thread>
#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include <driver/i2s.h>
#include <WiFi.h>
#include <esp_log.h>


#define ESP32
// firebase client
#include "Firebase_ESP_Client.h"

// provide the token generation process info.
#include "addons/TokenHelper.h"
// provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"


// contains definitions wifi SSID and PASSWORD
#include "../include/secrets.h"

// get sampler
#include "sampler/adc_sampler.h"

// definition which class shall be used
#define SUPPORT_ADC_I2S_SAMPLER

// push button for record
#define PUSH_BUTTON_PIN GPIO_NUM_33
#define ESP_INR_FLAG_DEFAULT 0


// base class of samplers
I2sSampler * i2sSampler;

// write task
TaskHandle_t i2sReadTaskHandle;


// Consts
//const int      NUM_OF_SAMPLES  = 16384;
const int      NUM_OF_SAMPLES  = 8092;
const int      SAMPLE_RATE     = 20000;

// config
i2s_config_t i2SConfig =
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
 * Resume task when button is pressed
 **********************************************************************/
FirebaseAuth   auth;
FirebaseConfig config;
FirebaseData   fireBaseData;
bool signupOk = false;



/**********************************************************************
 * Resume task when button is pressed
 **********************************************************************/
void IRAM_ATTR button_resume_task(void *arg)
{
  xTaskResumeFromISR(i2sReadTaskHandle);
}

/**********************************************************************
 * Setup push button
 **********************************************************************/
void setup_push_button()
{
  gpio_pad_select_gpio(PUSH_BUTTON_PIN);

  gpio_set_direction(PUSH_BUTTON_PIN, GPIO_MODE_INPUT);

  gpio_set_intr_type(PUSH_BUTTON_PIN, GPIO_INTR_POSEDGE);

  gpio_install_isr_service(ESP_INR_FLAG_DEFAULT);

  gpio_isr_handler_add(PUSH_BUTTON_PIN, button_resume_task, NULL);
}


/**********************************************************************
 * Setup wifi config and firebase client
 **********************************************************************/
void setup_wifi_and_firebase_client()
{
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

  // Assign the api key
  config.api_key = API_KEY;

  // Assign the RTDB URL
  config.database_url = DATABASE_URL;

  // Sign up
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOk = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  config.rtdb.upload_buffer_size = 1024;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}



/**********************************************************************
 * Task to write samples from ADC to firebase 
 * Start reading when button is pressed
 **********************************************************************/
void i2sReadAndSendTask(void *param)
{
  int samplesToRead = 1 * SAMPLE_RATE;

  // generic sampler
  I2sSampler* pSampler = (I2sSampler*) param;
  int16_t*    pSamples = (int16_t *)malloc(sizeof(uint16_t) * NUM_OF_SAMPLES );

#if 0
  int16_t*    pSamples = (int16_t *)malloc(sizeof(uint16_t) * samplesToRead);
#endif 
  
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
    // susped task until button gets pressed
    vTaskSuspend(NULL);
    Serial.println("Button Pressed!");

    int samplesRead = 0;
    int cnt = 0;
    
    Firebase.ready();

    // read 
    while ( samplesRead < samplesToRead) 
    {
      int numOfSamples = ( (samplesToRead - samplesRead) < NUM_OF_SAMPLES ) ? (samplesToRead - samplesRead) : NUM_OF_SAMPLES;
      // send as long as you want until client breaks up
      samplesRead += pSampler->read(pSamples, numOfSamples);
      Serial.printf("Returned: Samples read total: %d - First sample: %d\n", samplesRead, int(pSamples[0]));    

      
      // write to firebase in chunks - don't increment cnt for testing purpose
      // super SLOW - not really realtime - uploads 20s
      bool rtrn = Firebase.RTDB.setBlob(&fireBaseData, ("/test/MIC_DATA/chunk_" + std::to_string(cnt)).c_str(), (uint8_t*)(pSamples), sizeof(uint16_t) * NUM_OF_SAMPLES);
      Serial.printf("Set value: %s", ((rtrn) ? "ok" : fireBaseData.errorReason().c_str()));
    }
    

    Serial.println("Done");

    vTaskDelay(1000);
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
  setup_wifi_and_firebase_client();

  // setup ADC
  Serial.print("Setup ADC\n");

#ifdef SUPPORT_ADC_I2S_SAMPLER
  i2sSampler = new AdcSampler(ADC_UNIT_1, ADC1_CHANNEL_6, I2S_NUM_0, i2SConfig);
#endif

  i2sSampler->start();

  // setup task resume pin
  setup_push_button();

  // create task
  Serial.print("Setup Task\n");
  xTaskCreatePinnedToCore(i2sReadAndSendTask, "ADC Read and FireBase Write Task", 16384, i2sSampler, 1, &i2sReadTaskHandle, 1);

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