/********************************************************************************************
 * First Application
 * Only configure ADC and DAC via I2S
 * Stream to PC
 * When PC disconnects the stream, switch to receiving mode
 * PC starts sending the data back so esp can play it back
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