/********************************************************************************************
 * Second Application
 * Only start firebase client and task function with queue
 ********************************************************************************************/
#pragma once

#include <Arduino.h>
#include <FreeRTOSConfig.h>
#include <freertos/FreeRTOS.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>

// get fb client
#include "../fb_client/fb_client.h"

// get led class
#include "../led/rgb_led.h"

class CAppFbClient
{
public:
    CAppFbClient() = default;
    ~CAppFbClient() = default;

    void setup();
    
private:
    CFbClient* mFbClient { nullptr };
    CRgbLed*   mRgbLed   { nullptr };


    TaskHandle_t mTaskHandle;

    static void loop(void *param);
};