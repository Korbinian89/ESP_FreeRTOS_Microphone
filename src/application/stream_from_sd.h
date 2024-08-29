/********************************************************************************************
 * Sixth Application
 * Configure Firebase + LED + ADC + DAC + SD card
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
// get sd card 
#include "../sd_card/sd_card.h"

class CAppStreamFromSd
{
public:
    CAppStreamFromSd() = default;
    ~CAppStreamFromSd() = default;

    void setup();
    
private:
    // fb client
    CFbClient* mFbClient { nullptr };

    // rgb led
    CRgbLed* mRgbLed { nullptr };

    // sd card
    CSdCard* mSdCard { nullptr };

    // base class of samplers
    I2sSampler* mI2sAdcSampler { nullptr };

    // dac sampler
    I2sSampler* mI2sDacSampler { nullptr };

    // write & read handles
    TaskHandle_t mI2sWriteTaskHandle;
    TaskHandle_t mI2sReadTaskHandle;


    // setup push button
    void setup_push_button();

    // write and read task method
    static void i2s_read_and_send_task(void *param);
    static void i2s_recv_and_write_task(void *param);

    // interrupt
    static void button_resume_task(void *param);

};