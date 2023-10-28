/********************************************************************************************
 * First Application
 * Only configure ADC and DAC via I2S
 * Stream to PC
 * When PC disconnects the stream, switch to receiving mode
 * PC starts sending the data back so esp can play it back
 ********************************************************************************************/
#include <Arduino.h>
#include <FreeRTOSConfig.h>
#include <thread>
#include <freertos/FreeRTOS.h>

#include <driver/adc.h>
#include <driver/dac.h>
#include <driver/i2s.h>
#include <esp_system.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <esp_log.h>


// get dac sampler
#include "../sampler/dac_sampler.h"
// get adc sampler
#include "../sampler/adc_sampler.h"



class CAppStreamToHost
{
public:
    CAppStreamToHost() = default;
    ~CAppStreamToHost() = default;

    void setup();
    
private:
    // wifi server - provides samples
    WiFiServer * mWifiServer { nullptr };

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
};