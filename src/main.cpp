// App compile switch
#define APP6_STREAM_FROM_SD


#if defined(APP1_STREAM_TO_HOST)
#include "application/stream_to_host.h"
CAppStreamToHost* pAppStreamToHost = nullptr;

#elif defined(APP2_FB_CLIENT)
#include "application/fb_client_task_queue.h"
CAppFbClient* pAppFbClient = nullptr;

#elif defined(APP3_STREAM_TO_FB)
#include "application/stream_to_fb.h"
CAppStreamToFb* pAppStreamToFb = nullptr;

#elif defined(APP4_STREAM_TO_SD)
#include "application/stream_to_sd.h"
CAppStreamToSd* pAppStreamToSd = nullptr;

#elif defined(APP5_SD_TO_FB)
// Just test SD file upload to firebase
#include "application/sd_to_fb.h"
CAppSdToFb* pAppSdToFb = nullptr;

#elif defined(APP6_STREAM_FROM_SD)
#include "application/stream_from_sd.h"
CAppStreamFromSd* pAppStreamFromSd = nullptr;

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

#elif defined(APP3_STREAM_TO_FB)
  pAppStreamToFb = new CAppStreamToFb();
  pAppStreamToFb->setup();

#elif defined(APP4_STREAM_TO_SD)
  pAppStreamToSd = new CAppStreamToSd();
  pAppStreamToSd->setup();

#elif defined(APP5_SD_TO_FB)
  pAppSdToFb = new CAppSdToFb();
  pAppSdToFb->setup();

#elif defined(APP6_STREAM_FROM_SD)
  pAppStreamFromSd = new CAppStreamFromSd();
  pAppStreamFromSd->setup();

#endif

  Serial.printf("Main Setup finished\n");
}


/**********************************************************************
 * Loopy Loop
 **********************************************************************/
void loop() 
{
}