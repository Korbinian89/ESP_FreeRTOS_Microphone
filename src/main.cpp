// App compile switch
#define APP2_FB_CLIENT


#if defined(APP1_STREAM_TO_HOST)
#include "application/stream_to_host.h"
CAppStreamToHost* pAppStreamToHost = nullptr;

#elif defined(APP2_FB_CLIENT)
#include "application/fb_client_task_queue.h"
CAppFbClient* pAppFbClient     = nullptr;

#endif

/**********************************************************************
 * Setup system
 **********************************************************************/
void setup()
{
  Serial.begin(115200);

  Serial.printf("ESP-IDF Version %d.%d.%d \r\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
  Serial.printf("Ardunio Version %d.%d.%d \r\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);

  Serial.printf("Starting Application\n");
  
#if defined(APP1_STREAM_TO_HOST)
  pAppStreamToHost = new CAppStreamToHost();
  pAppStreamToHost->setup();

#elif defined(APP2_FB_CLIENT)
  pAppFbClient = new CAppFbClient();
  pAppFbClient->setup();

#endif

  Serial.printf("Main Setup finished\n");
}


/**********************************************************************
 * Loopy Loop
 **********************************************************************/
void loop() 
{
}