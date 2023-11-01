/********************************************************************************************
 * FireBase client class
 ********************************************************************************************/
#define FIREBASE_JOB_QUEUE_LENGTH 10


#include "fb_client.h"

#include "../../include/secrets.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Interface to RGB LED
#include "led/i_rgb_led.h"

#include <string>

// Static init
QueueHandle_t CFbClient::mRecvJobQueue;


/********************************************************************************************
 * Initialize firebase connection and create free RTOS task
 ********************************************************************************************/
void CFbClient::setup(IRgbLed* iRgbLed)
{
  // Get interface pointer to LED
  mRgbLed = iRgbLed;

  // FreeRTOS
  // init recv job queue and task via free rtos
  // - rule of thumb, increase stack until application doesn't crash anymore
  mRecvJobQueue = xQueueCreate(FIREBASE_JOB_QUEUE_LENGTH,sizeof(SJob));
  xTaskCreatePinnedToCore(CFbClient::recv_job_task, "FB Client Recv Job Task", 12288, this, 1, &mRecvJobTaskHandle, 1);


  // FireBase
  // init firebase connection, example initialization code from repository
  // assign the api key
  mConfig.api_key = API_KEY;

  // assign the RTDB URL
  mConfig.database_url = DATABASE_URL;

  // sign up
  if (Firebase.signUp(&mConfig, &mAuth, "", ""))
  {
    Serial.println("ok");
  }
  else
  {
    Serial.printf("%s\n", mConfig.signer.signupError.message.c_str());
  }

  // Assign the callback function for the long running token generation task
  mConfig.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&mConfig, &mAuth);
  Firebase.reconnectWiFi(true);

  // connect colors first
  if (!Firebase.RTDB.beginMultiPathStream(&mStreamColor, "test/LED_COLOR"))
    Serial.printf("sream begin error, %s\n\n", mStreamColor.errorReason().c_str());

  Firebase.RTDB.setMultiPathStreamCallback(&mStreamColor, color_stream_callback, stream_timeout);

  // then state of LED
  if (!Firebase.RTDB.beginStream(&mStreamState, "test/LED_STATE"))
    Serial.printf("sream begin error, %s\n\n", mStreamState.errorReason().c_str());

  Firebase.RTDB.setStreamCallback(&mStreamState, state_stream_callback, stream_timeout);
}


/********************************************************************************************
 * Add task for uploading the LED state
 ********************************************************************************************/
void CFbClient::upload_state()
{
  printf("Upload State - Add Job\n");
  SJob job;
  job.mId = SJob::LED_STATE_SEND;

  xQueueSend(mRecvJobQueue, &job, portMAX_DELAY);
}


/********************************************************************************************
 * Add task for uploading the LED color
 ********************************************************************************************/
void CFbClient::upload_color()
{
  printf("Upload Color - Add Job\n");
  SJob job;
  job.mId = SJob::LED_COLOR_SEND;
  
  xQueueSend(mRecvJobQueue, &job, portMAX_DELAY);
}

/********************************************************************************************
 * Free RTOS task
 ********************************************************************************************/
void CFbClient::recv_job_task(void *param)
{
  auto pThis = static_cast<CFbClient*>(param);

  while(1)
  {
    SJob job;
    FirebaseJson json;
    FirebaseData fbdo;

    if (xQueueReceive(mRecvJobQueue, &job, portMAX_DELAY) == pdPASS && Firebase.ready())
    {
      // check job
      switch(job.mId)
      {
        case SJob::LED_STATE_RECV:
          // set state
          if (pThis->mRgbLed)
          {
            pThis->mRgbLed->set_state(job.mState);
          }
          break;
        case SJob::LED_COLOR_RECV:
          // set color
          if (pThis->mRgbLed)
          {
            pThis->mRgbLed->set_color(job.mColor, job.mColorValue);
          }
          break;
        case SJob::AUDIO_RECV:
          // play audio
          break;
        case SJob::LED_STATE_SEND:
          // upload state to firebase
          if (pThis->mRgbLed)
          {
            printf("Upload State - Set Bool\n");
            Firebase.RTDB.setBool(&fbdo, "test/LED_STATE", pThis->mRgbLed->get_state());
          }
          break;
        case SJob::LED_COLOR_SEND:
          for (int i = 0; i < static_cast<int>(EColor::NUM_OF_COLORS); ++i)
          {
            if (pThis->mRgbLed)
            {
              printf("Upload Color - Add path %s to JSON\n", sColorToString[EColor(i)]);
              json.set(sColorToString[EColor(i)], pThis->mRgbLed->get_color(EColor(i)));
            }
          }
          printf("Upload Color - Set JSON\n");
          Firebase.RTDB.setJSON(&fbdo, "test/LED_COLOR", &json);
          break;
        default:
          break;
      }
    }
  }
}


/********************************************************************************************
 * Simple data stream for LED state
 ********************************************************************************************/
void CFbClient::state_stream_callback(FirebaseStream iData)
{
  Serial.println("StateStreamCallback");
  Serial.printf("Received stream path: %s, event path: %s, data type: %s, event type, %s\n",
                iData.streamPath().c_str(),
                iData.dataPath().c_str(),
                iData.dataType().c_str(),
                iData.eventType().c_str());
  printResult(iData); // see addons/RTDBHelper.h

  // This is the size of stream payload received (current and max value)
  // Max payload size is the payload size under the stream path since the stream connected
  // and read once and will not update until stream reconnection takes place.
  // This max value will be zero as no payload received in case of ESP8266 which
  // BearSSL reserved Rx buffer size is less than the actual stream payload.
  Serial.printf("Received stream payload size: %d (Max. %d)\n", iData.payloadLength(), iData.maxPayloadLength());

  // Due to limited of stack memory, do not perform any task that used large memory here
  SJob job;
  job.mId    = SJob::LED_STATE_RECV;
  job.mState = iData.boolData();

  xQueueSend(mRecvJobQueue, &job, portMAX_DELAY);

  Serial.println();
}


/********************************************************************************************
 * Multi stream, for each color
 ********************************************************************************************/
void CFbClient::color_stream_callback(MultiPathStream iData)
{
  Serial.println("ColorStreamCallback");
  Serial.printf("Received path: %s, event: %s, type: %s, value: %s%s", iData.dataPath.c_str(), iData.eventType.c_str(), iData.type.c_str(), iData.value.c_str(), "\n");

  // This is the size of stream payload received (current and max value)
  // Max payload size is the payload size under the stream path since the stream connected
  // and read once and will not update until stream reconnection takes place.
  // This max value will be zero as no payload received in case of ESP8266 which
  // BearSSL reserved Rx buffer size is less than the actual stream payload.
  Serial.printf("Received stream payload size: %d (Max. %d)\n", iData.payloadLength(), iData.maxPayloadLength());

  // update all colors at once -> does this make sense? 
  // - Yes it's possible to update multiple colors at once 
  // - Call get on stream for each path
  for (int i = 0; i < static_cast<int>(EColor::NUM_OF_COLORS); i++)
  {
    if (iData.get(sColorToString[EColor(i)]))
    {
      Serial.printf("Received child path: %s, event: %s, type: %s, value: %s%s", iData.dataPath.c_str(), iData.eventType.c_str(), iData.type.c_str(), iData.value.c_str(), "\n");
      
      SJob job;
      job.mId         = SJob::LED_COLOR_RECV;
      job.mColor      = EColor(i);
      job.mColorValue = iData.value.toInt();
      xQueueSend(mRecvJobQueue, &job, portMAX_DELAY);
    }
  }
  Serial.println();
}


/********************************************************************************************
 * Stream timeout
 ********************************************************************************************/
void CFbClient::stream_timeout(bool iTimeout)
{
  Serial.printf("StreamTimeOut: %s\n", (iTimeout) ? "True" : "False");
}