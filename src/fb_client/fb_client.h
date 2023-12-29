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
#include "firestore/FB_Firestore.h"

/* Forward Declarations *****************************************************************/
class IRgbLed;
class ISdCard;

/* Job Definition ***********************************************************************/
struct SJob
{
    enum EJobId 
    {
        UNKNOWN    = 0
    ,   LED_STATE_RECV  = 1
    ,   LED_COLOR_RECV  = 2
    ,   AUDIO_RECV      = 3
    ,   LED_STATE_SEND  = 4
    ,   LED_COLOR_SEND  = 5
    ,   AUDIO_SEND      = 6
    };
    EJobId   mId          = EJobId::UNKNOWN; // JOB ID defines which member below is evalulated - think of something better
    bool     mState       = 0;               // LED_STATE: State of the LED
    EColor   mColor       = EColor::BLUE;    // LED_COLOR: Which color was changed
    int      mColorValue  = 0;               // LED_COLOR: value between 0-255 for given color
    String   mPathToAudio = "";              // AUDIO_RECV/SEND: Path on SD to audio file + name
};


/* Class Definition ***********************************************************************/
class CFbClient
{
public:
    CFbClient() = default;
    ~CFbClient() = default;

    void setup(IRgbLed* iRgbLed);
    void setup(IRgbLed* iRgbLed, ISdCard* iSdCard);

    void upload_state();
    void upload_color();
    void upload_audio();
    void download_audio();

    int upload_audio(uint8_t* iData, size_t iLen, int iIdx);
    int download_audio(uint8_t* oData, size_t iLen, int iIdx);

    void test();
    void init_test();

private:
    IRgbLed* mRgbLed { nullptr };
    ISdCard* mSdCard { nullptr };

    static QueueHandle_t mRecvJobQueue;
    TaskHandle_t         mRecvJobTaskHandle;

    FirebaseData   mStreamState;
    FirebaseData   mStreamColor;
    FirebaseData   mAudioFbDataSend;
    FirebaseData   mAudioFbDataRecv;
    FirebaseAuth   mAuth;
    FirebaseConfig mConfig;

    static void state_stream_callback(FirebaseStream iData);
    static void color_stream_callback(MultiPathStream iData);
    static void stream_timeout(bool iTimeout);

    static void rtdb_download_callback(RTDB_DownloadStatusInfo info);
    static void rtdb_upload_callback(RTDB_UploadStatusInfo info);

    // Remove static and use wrapper: https://www.freertos.org/FreeRTOS_Support_Forum_Archive/July_2010/freertos_Is_it_possible_create_freertos_task_in_c_3778071.html
    static void recv_job_task(void *param);
};
