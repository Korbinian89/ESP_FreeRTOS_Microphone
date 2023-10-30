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



class CAppFbClient
{
public:
    CAppFbClient() = default;
    ~CAppFbClient() = default;

    void setup();
    
private:
    CFbClient* mFbClient { nullptr };
};