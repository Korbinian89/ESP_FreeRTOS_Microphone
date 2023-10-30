/********************************************************************************************
 * Second Application
 ********************************************************************************************/
#include "fb_client_task_queue.h"

// contains definitions wifi SSID and PASSWORD
#include "../../include/secrets.h"


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

  // Fb Client
  Serial.print("Setup FB Client\n");
  mFbClient = new CFbClient();
  mFbClient->setup();

  Serial.print("Setup - done\n");
}


