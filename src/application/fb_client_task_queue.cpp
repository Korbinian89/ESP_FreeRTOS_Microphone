/********************************************************************************************
 * Second Application
 ********************************************************************************************/
#include "fb_client_task_queue.h"

// contains definitions wifi SSID and PASSWORD
#include "../../include/secrets.h"



/********************************************************************************************
 * Setup
 * - Create LED
 * - Create FB Client
 * - Create App main loop
 ********************************************************************************************/
void CAppFbClient::setup()
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

  // Rgb LED
  Serial.print("Setup RGB LED\n");
  mRgbLed = new CRgbLed();
  mRgbLed->setup();

  // Fb Client and pass interface to firebase client, dependency injection
  Serial.print("Setup FB Client\n");
  mFbClient = new CFbClient();
  mFbClient->setup(mRgbLed);

  // Setup application main loop
  Serial.print("Setup application loop\n");
  xTaskCreatePinnedToCore(CAppFbClient::loop, "App loop", 4096, this, 1, &mTaskHandle, 0);

  Serial.print("Setup - done\n");
}


/********************************************************************************************
 * App main loop
 * - Every 10s:
 *   - Change state of LED and Upload
 *   - Get and Set all colors of LED +20 and Upload
 ********************************************************************************************/
void CAppFbClient::loop(void *param)
{
  auto pThis = static_cast<CAppFbClient*>(param);

  while(1)
  {
    delay(10000);
    
    pThis->mRgbLed->set_state(!pThis->mRgbLed->get_state());
    pThis->mFbClient->upload_state();

    for (int i = 0; i < EColor::NUM_OF_COLORS; ++i)
    {
      auto val = pThis->mRgbLed->get_color(EColor(i)) + 20;
      if (val > 255)
      {
        val = 0;
      }
      pThis->mRgbLed->set_color(EColor(i), val);
    }
    pThis->mFbClient->upload_color();
  } 
}
