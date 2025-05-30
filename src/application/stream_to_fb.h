/********************************************************************************************
 * Third Application
 * Configure Firebase + LED + ADC + DAC
 * Stream 1s to FB when button is pressed wait 3s and receive uploaded data
 ********************************************************************************************/
#pragma once

#include <Arduino.h>
#include <FreeRTOSConfig.h>
#include <freertos/FreeRTOS.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>

// get dac sampler
#include "../sampler/dac_sampler.h"
// get adc sampler
#include "../sampler/adc_sampler.h"
// get rgb led
#include "../led/rgb_led.h"
// get fb client
#include "../fb_client/fb_client.h"

class CAppStreamToFb
{
public:
    CAppStreamToFb() = default;
    ~CAppStreamToFb() = default;

    void setup();
    
private:
    // fb client
    CFbClient * mFbClient { nullptr };

    // rgb led
    CRgbLed * mRgbLed   { nullptr };

    // base class of samplers
    I2sSampler * mI2sAdcSampler { nullptr };

    // dac sampler
    I2sSampler * mI2sDacSampler { nullptr };

    // write & read handles
    TaskHandle_t mI2sWriteTaskHandle;
    TaskHandle_t mI2sReadTaskHandle;

    // write and read task method
    static void i2s_read_and_send_task(void *param);
    static void i2s_recv_and_write_task(void *param);

    // setup push button
    void setup_push_button();

    // interrupt
    static void button_resume_task(void *param);

};