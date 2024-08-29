/********************************************************************************************
 * Fourth Application
 * Configure Firebase + LED + ADC + DAC + SD card
 * NOTE: TEST ONLY FILE UPLOAD TO FIREBASE 
 * NOTHING MORE - DUMMY APP
 ********************************************************************************************/
#include "sd_to_fb.h"

// config for i2s
#include "../config/app_config.h"

// config for led
#include "../config/led_config.h"

// contains definitions wifi SSID and PASSWORD
#include "../../include/secrets.h"


void CAppSdToFb::setup()
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
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  // Rgb LED
  Serial.print("Setup RGB LED\n");
  mRgbLed = new CRgbLed();
  mRgbLed->setup();

  // Fb Client and pass interface to firebase client, dependency injection
  Serial.print("Setup FB Client\n");
  mFbClient = new CFbClient();
  mFbClient->setup(mRgbLed, mSdCard);

  // loop task
  xTaskCreatePinnedToCore(CAppSdToFb::loop_task, "Loop Task", 16384, this, 1, &mLoopTaskHandle, 1);
  
  // setup push button
  setup_push_button();
  Serial.print("Setup Push Button\n");

  Serial.print("Setup - done\n");
}


void CAppSdToFb::loop_task(void *param)
{
  auto        pThis     = static_cast<CAppSdToFb*>(param);
  CFbClient*  pFbClient = pThis->mFbClient;
  CSdCard*    pSdCard   = pThis->mSdCard;

  while(true)
  {
    // suspend this until button gets pressed
    vTaskSuspend(NULL);
    pFbClient->test();
  }  
}

/**********************************************************************
 * Resume task when button is pressed
 **********************************************************************/
void IRAM_ATTR CAppSdToFb::button_resume_task(void *param)
{
  auto pThis = static_cast<CAppSdToFb*>(param);

  xTaskResumeFromISR(pThis->mLoopTaskHandle);
}

/**********************************************************************
 * Setup push button
 **********************************************************************/
void CAppSdToFb::setup_push_button()
{
  gpio_pad_select_gpio(PUSH_BUTTON_PIN);

  gpio_set_direction(PUSH_BUTTON_PIN, GPIO_MODE_INPUT);

  gpio_set_intr_type(PUSH_BUTTON_PIN, GPIO_INTR_POSEDGE);

  gpio_install_isr_service(ESP_INR_FLAG_DEFAULT);

  gpio_isr_handler_add(PUSH_BUTTON_PIN, CAppSdToFb::button_resume_task, this);
}