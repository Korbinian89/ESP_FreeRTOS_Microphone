/********************************************************************************************
 * Firebase client
 * - Receive LED state/color and audio from database
 * - Send LED state/color and audio to database 
 ********************************************************************************************/
#pragma once

#include <map>
#include <queue>

#include "config/led_config.h"
#include "Firebase_ESP_Client.h"

/* Forward Declarations *****************************************************************/
class IRgbLed;


/* Job Definition ***********************************************************************/
struct SJob
{
    enum EJobId 
    {
        UNKNOWN    = 0
    ,   LED_STATE  = 1
    ,   LED_COLOR  = 2
    ,   AUDIO_RECV = 3
    };
    EJobId   mId         = EJobId::UNKNOWN; // JOB ID defines which member below is evalulated - think of something better
    bool     mState      = 0;               // LED_STATE: State of the LED
    EColor   mColor      = EColor::BLUE;    // LED_COLOR: Which color was changed
    int      mColorValue = 0;               // LED_COLOR: value between 0-255 for given color
    uint8_t* mData       = nullptr;         // AUDIO_RECV: pointer to shared buffer
};


/* Class Definition ***********************************************************************/
class CFbClient
{
public:
    CFbClient() = default;
    ~CFbClient() = default;

    void setup(IRgbLed* iRgbLed);

private:
    IRgbLed* mRgbLed { nullptr };

    static QueueHandle_t mRecvJobQueue;
    TaskHandle_t         mRecvJobTaskHandle;

    FirebaseData   mStreamState;
    FirebaseData   mStreamColor;
    FirebaseAuth   mAuth;
    FirebaseConfig mConfig;

    static void state_stream_callback(FirebaseStream iData);
    static void color_stream_callback(MultiPathStream iData);
    static void stream_timeout(bool iTimeout);

    // Remove static and use wrapper: https://www.freertos.org/FreeRTOS_Support_Forum_Archive/July_2010/freertos_Is_it_possible_create_freertos_task_in_c_3778071.html
    static void recv_job_task(void *param);
};
