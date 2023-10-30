/********************************************************************************************
 * FireBase client class
 ********************************************************************************************/
#define FIREBASE_JOB_QUEUE_LENGTH 10


#include "fb_client.h"

#include "../../include/secrets.h"

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Static init
QueueHandle_t CFbClient::mRecvJobQueue;


/********************************************************************************************
 * Initialize firebase connection and create free RTOS task
 ********************************************************************************************/
void CFbClient::setup()
{
  // init recv job queue and task via free rtos
  mRecvJobQueue = xQueueCreate(FIREBASE_JOB_QUEUE_LENGTH,sizeof(SJob));
  xTaskCreatePinnedToCore(CFbClient::recv_job_task, "FB Client Recv Job Task", 4096, this, 1, &mRecvJobTaskHandle, 1);

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
 * Free RTOS task
 ********************************************************************************************/
void CFbClient::recv_job_task(void *param)
{
  auto pThis = static_cast<CFbClient*>(param);

  while(1)
  {
    SJob job;
    if (xQueueReceive(mRecvJobQueue, &job, portMAX_DELAY) == pdPASS)
    {
      // check job
      switch(job.mId)
      {
        case SJob::LED_STATE:
          // set state
          Serial.printf("Set state to: %s\n", ( job.mState ? "On" : "Off"));
          break;
        case SJob::LED_COLOR:
          // set color
          Serial.printf("Set color: %s to %d\n", sColorToString[job.mColor], job.mColorValue);
          break;
        case SJob::AUDIO_RECV:
          // play audio
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
  job.mId    = SJob::LED_STATE;
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
  for ( size_t i = 0; i < (sizeof(mColors) / sizeof(int)); i++)
  {
    if ( iData.get(sColorToString[EColor(i)]) )
    {
      Serial.printf("Received child path: %s, event: %s, type: %s, value: %s%s", iData.dataPath.c_str(), iData.eventType.c_str(), iData.type.c_str(), iData.value.c_str(), "\n");
      
      SJob job;
      job.mId         = SJob::LED_COLOR;
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