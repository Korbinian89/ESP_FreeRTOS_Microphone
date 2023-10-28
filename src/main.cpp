
// First app
#include "application/stream_to_host.h"

CAppStreamToHost* pAppStreamToHost = nullptr;


/**********************************************************************
 * Setup system
 **********************************************************************/
void setup()
{
  Serial.begin(115200);

  Serial.printf("ESP-IDF Version %d.%d.%d \r\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
  Serial.printf("Ardunio Version %d.%d.%d \r\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);

  Serial.printf("Starting Application\n");
  
  pAppStreamToHost = new CAppStreamToHost();
  pAppStreamToHost->setup();

  Serial.printf("Main Setup finished\n");
}


/**********************************************************************
 * Loopy Loop
 **********************************************************************/
void loop() 
{
}