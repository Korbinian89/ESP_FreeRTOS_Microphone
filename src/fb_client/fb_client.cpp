/********************************************************************************************
 * FireBase client class
 ********************************************************************************************/
#define FIREBASE_JOB_QUEUE_LENGTH 10


#include "fb_client.h"

#include "../../include/secrets.h"

#include "config/app_config.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
// Provide sd helper for FB
#include "addons/SDHelper.h"


// Interface to RGB LED
#include "led/i_rgb_led.h"

// Interface to SD Card
#include "sd_card/i_sd_card.h"

#include <string>

// Static init
QueueHandle_t CFbClient::mRecvJobQueue;
FirebaseData CFbClient::mAudioFbData;

/********************************************************************************************
 * Initialize firebase connection and create free RTOS task
 ********************************************************************************************/
void CFbClient::setup(IRgbLed* iRgbLed)
{
  setup(iRgbLed, nullptr);
}

/********************************************************************************************
 * Initialize firebase connection and create free RTOS task
 ********************************************************************************************/
void CFbClient::setup(IRgbLed* iRgbLed, ISdCard* iSdCard)
{
  // Get interface pointer to LED
  mRgbLed = iRgbLed;

  // Get interface pointer to Sd card
  mSdCard = iSdCard;

  Serial.println("Firebase Create Queue");

  // FreeRTOS
  // init recv job queue and task via free rtos
  // - rule of thumb, increase stack until application doesn't crash anymore
  mRecvJobQueue = xQueueCreate(FIREBASE_JOB_QUEUE_LENGTH,sizeof(SJob));
  xTaskCreatePinnedToCore(CFbClient::recv_job_task, "FB Client Recv Job Task", 12288, this, 2, &mRecvJobTaskHandle, 1);


  // FireBase
  // init firebase connection, example initialization code from repository
  // assign the api key
  mConfig.api_key = API_KEY;

  // assign the RTDB URL
  mConfig.database_url = DATABASE_URL;

  Serial.println("Firebase SignUp");
  //mAuth.user.email = USER_EMAIL;
  //mAuth.user.password = USER_PASSWORD;


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
  

  // Audio data - 2x DMA buffer a 1024 samples and each sample 2 byte -> 1024 * 2 * 2 = 4096
  mAudioFbData.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  // fcs buffer
  mConfig.fcs.download_buffer_size = 2048;
  mConfig.fcs.upload_buffer_size = 512;

  Serial.println("Firebase Begin");
  Firebase.begin(&mConfig, &mAuth);

  // init sd card access if necessary
  if (mSdCard)
  {
    if (Firebase.sdBegin(SPI_CS_PIN, mSdCard->get_spi()))
    {
      Serial.println("Firebase SD success");
    }
    else
    {
      Serial.println("Firebase SD failed");
    }
  }
  
  // reconnector wifi
  Serial.println("Firebase Reconnect WiFi");
  Firebase.reconnectNetwork(true);

  //init_test();

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
  Serial.printf("Upload State - Add Job\n");
  SJob job;
  job.mId = SJob::LED_STATE_SEND;

  xQueueSend(mRecvJobQueue, &job, portMAX_DELAY);
}


/********************************************************************************************
 * Add task for uploading the LED color
 ********************************************************************************************/
void CFbClient::upload_color()
{
  Serial.printf("Upload Color - Add Job\n");
  SJob job;
  job.mId = SJob::LED_COLOR_SEND;
  
  xQueueSend(mRecvJobQueue, &job, portMAX_DELAY);
}


/********************************************************************************************
 * Add task for uploading the audio file from SD to firebase storage
 ********************************************************************************************/
void CFbClient::upload_audio_to_firebase_storage()
{
  #if 0
  Serial.printf("Upload Audio - Add Job\n");
  SJob job;
  job.mId = SJob::AUDIO_SEND;
  job.mPathToAudio = "/recording.bin";
  
  xQueueSend(mRecvJobQueue, &job, portMAX_DELAY);
  #endif


  Serial.printf("Upload File\n");

  if (!Firebase.Storage.upload(&mAudioFbData, STORAGE_URL, UPLOAD_FILE_NAME.c_str(), mem_storage_type_sd, FS_FILE_NAME.c_str(), "application/octet-stream" , fcsUploadCallback /* callback function*/))
    Serial.println(mAudioFbData.errorReason());
  else
    Serial.printf("Upload File - Succeeded\n");
}


/********************************************************************************************
 * Add task for downloading the audio file from firebase storage to SD 
 ********************************************************************************************/
void CFbClient::download_audio_from_firebase_storage()
{
  #if 0
  printf("Download Audio-  Add Job\n");
  SJob job;
  job.mId = SJob::AUDIO_RECV;
  job.mPathToAudio = "/recording.bin";
  
  xQueueSend(mRecvJobQueue, &job, portMAX_DELAY);
  #endif

  Serial.printf("Download File\n");

  if (!Firebase.Storage.download(&mAudioFbData, STORAGE_URL, FS_FILE_NAME.c_str(), DOWNLOAD_FILE_NAME.c_str(), mem_storage_type_sd, fcsDownloadCallback /* callback function*/))
    Serial.println(mAudioFbData.errorReason());
  else
  {
    Serial.printf("Download File - Succeeded\n");
    Serial.printf("HTTP Code: %d\n" + mAudioFbData.httpCode());
  }
}


/********************************************************************************************
 * Add task for uploading the audio file from SD
 ********************************************************************************************/
void CFbClient::upload_audio()
{
  #if 0
  Serial.printf("Upload Audio - Add Job\n");
  SJob job;
  job.mId = SJob::AUDIO_SEND;
  job.mPathToAudio = "/recording.bin";
  
  xQueueSend(mRecvJobQueue, &job, portMAX_DELAY);
  #endif


  Serial.printf("Upload File\n");

  // manually  triggered to upload audio from SD
  if (Firebase.RTDB.setFile(&mAudioFbData, mem_storage_type_sd, "test/sample/raw", "/sample_16kHz_signed_16bit.raw", rtdb_upload_callback))
  {
    Serial.printf("Upload File - Succeeded\n");
  }
  else
  {
    Serial.printf("Upload File - Failed\n");
  }
}


/********************************************************************************************
 * Add task for downloading the audio file to SD
 ********************************************************************************************/
void CFbClient::download_audio()
{
  #if 0
  printf("Download Audio-  Add Job\n");
  SJob job;
  job.mId = SJob::AUDIO_RECV;
  job.mPathToAudio = "/recording.bin";
  
  xQueueSend(mRecvJobQueue, &job, portMAX_DELAY);
  #endif

  Serial.printf("Download File\n");

  // manually  triggered to upload audio from SD
  if (Firebase.RTDB.getFile(&mAudioFbData, mem_storage_type_sd, "test/sample/raw", "/sample_16kHz_signed_16bit_download.raw", rtdb_download_callback))
  {
    Serial.printf("Download File - Succeeded\n");
    Serial.printf("HTTP Code: %d\n" + mAudioFbData.httpCode());
  }
  else
  {
    Serial.println(mAudioFbData.errorReason());
    Serial.printf("Download File - Failed\n");
  }

}


/********************************************************************************************
 * DOES NOT WORK - TAKES TOO LONG - NOT CAPABLE OF REALTIME
 * Upload audio data directly via async blob
 * iLen -> num of bytes
 * 
 ********************************************************************************************/
int CFbClient::upload_audio(uint8_t* iData, size_t iLen, int iIdx)
{
  int bytesWritten = 0;

  uint8_t* buf = (uint8_t *)malloc(iLen);
  memcpy(buf, iData, iLen);

  Serial.println("Upload audio firebase start");

  if (Firebase.ready())
  {
    if (Firebase.RTDB.setBlob(&mAudioFbData, ("/test/MIC_DATA/chunk_" + std::to_string(iIdx)).c_str(), buf, iLen))
    {
      bytesWritten += iLen;
      Serial.println("Upload audio firebase success");
    }
    else
    {
      Serial.printf("Upload audio failed: %s\n", mAudioFbData.errorReason().c_str());
    }
  }
  else
  {
    Serial.println("Upload audio firebase not ready");
  }

  free(buf);
  return bytesWritten;
}


/********************************************************************************************
 *  DOES NOT WORK - TAKES TOO LONG - NOT CAPABLE OF REALTIME
 *  Download audio data directly via blob
 ********************************************************************************************/
int CFbClient::download_audio(uint8_t* oData, size_t iLen, int iIdx)
{
  int bytesRead = 0;

  std::vector<uint8_t> fbDataVec;

  if(Firebase.ready())
  {
    if (Firebase.RTDB.getBlob(&mAudioFbData, ("/test/MIC_DATA/chunk_" + std::to_string(iIdx)).c_str()))
    {
      auto fbDataVec = mAudioFbData.blobData();
      for (size_t i = 0; i < fbDataVec->size(); ++i)
      {
        oData[i] = fbDataVec->at(i);
      }
      bytesRead += iLen;
    }
    else
    {
      Serial.printf("Download audio failed: %s\n", mAudioFbData.errorReason().c_str());
    }
  }
  else
  {
    Serial.println("Download audio firebase not ready");
  }
  return bytesRead;
}




void CFbClient::test()
{
  // Firebase.ready() should be called repeatedly to handle authentication tasks.
  if (Firebase.ready())
  {
    // File name must be in 8.3 DOS format (max. 8 bytes file name and 3 bytes file extension)
    Serial.println("\nSet file...");

    if (!Firebase.Storage.upload(&mAudioFbData, STORAGE_URL, "/file1.txt", mem_storage_type_sd, "file.txt", "text/plain" , fcsUploadCallback /* callback function*/))
      Serial.println(mAudioFbData.errorReason());

    Serial.println("\nGet file...");
    if (!Firebase.Storage.download(&mAudioFbData, STORAGE_URL, "file.txt",  "/file2.txt", mem_storage_type_sd, fcsDownloadCallback /* callback function*/))
      Serial.println(mAudioFbData.errorReason());

    if (mAudioFbData.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK)
    {
      File file;
      file = DEFAULT_SD_FS.open("/file2.txt", "r");

      int i = 0;

      while (file.available())
      {
        //if (i > 0 && i % 16 == 0)
        //  Serial.println();

        uint8_t v = file.read();

        //if (v < 16)
        //  Serial.print("0");

        //Serial.print(v, HEX);
        //Serial.print(" ");
        i++;
      }
      Serial.print("File read");
      Serial.println();
      file.close();
    }
  }
  init_test();
}


void CFbClient::init_test()
{
  // Delete demo files
  if (DEFAULT_SD_FS.exists("/file1.txt"))
    DEFAULT_SD_FS.remove("/file1.txt");

  if (DEFAULT_SD_FS.exists("/file2.txt"))
    DEFAULT_SD_FS.remove("/file2.txt");

  File file = DEFAULT_SD_FS.open("/file1.txt", "w");

  uint8_t v = 0;
  for (int i = 0; i < 1000000; i++)
  {
    file.write(v);
    v++;
  }

  file.close();
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

    if (/*Firebase.ready() &&*/ xQueueReceive(mRecvJobQueue, &job, portMAX_DELAY) == pdPASS)
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
          // manually triggered download audio to SD
          printf("Download File\n");

          // manually  triggered to upload audio from SD
          if (Firebase.RTDB.getFile(&pThis->mAudioFbData, mem_storage_type_sd, "test/AUDIO", job.mPathToAudio))
          {
            printf("Download File - Succeeded\n");
          }
          else
          {
            printf("Download File - Failed\n");
          }
          break;
        case SJob::AUDIO_SEND:
          printf("Upload File\n");

          // manually  triggered to upload audio from SD
          if (Firebase.RTDB.setFile(&pThis->mAudioFbData, mem_storage_type_sd, "test/AUDIO", job.mPathToAudio))
          {
            printf("Upload File - Succeeded\n");
          }
          else
          {
            printf("Upload File - Failed\n");
          }
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


/********************************************************************************************
 * Download callback
 ********************************************************************************************/
void CFbClient::rtdb_download_callback(RTDB_DownloadStatusInfo info)
{
  if (info.status == firebase_rtdb_download_status_init)
  {
    Serial.printf("Downloading file %s (%d) to %s\n", info.remotePath.c_str(), info.size, info.localFileName.c_str());
  }
  else if (info.status == firebase_rtdb_download_status_download)
  {
    Serial.printf("Downloaded %d%s\n", (int)info.progress, "%");
  }
  else if (info.status == firebase_rtdb_download_status_complete)
  {
    Serial.println("Download completed\n");
  }
  else if (info.status == firebase_rtdb_download_status_error)
  {
    Serial.printf("Download failed, %s\n", info.errorMsg.c_str());
  }
}


/********************************************************************************************
 * Upload callback
 ********************************************************************************************/
void CFbClient::rtdb_upload_callback(RTDB_UploadStatusInfo info)
{
  if (info.status == firebase_rtdb_upload_status_init)
  {
    Serial.printf("Uploading file %s (%d) to %s\n", info.localFileName.c_str(), info.size, info.remotePath.c_str());
  }
  else if (info.status == firebase_rtdb_upload_status_upload)
  {
    Serial.printf("Uploaded %d%s\n", (int)info.progress, "%");
  }
  else if (info.status == firebase_rtdb_upload_status_complete)
  {
    Serial.println("Upload completed\n");
  }
  else if (info.status == firebase_rtdb_upload_status_error)
  {
    Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
  }
}


/********************************************************************************************
 * The Firebase Storage upload callback function
 ********************************************************************************************/
void CFbClient::fcsUploadCallback(FCS_UploadStatusInfo info)
{
    if (info.status == firebase_fcs_upload_status_init)
    {
        Serial.printf("Uploading file %s (%d) to %s\n", info.localFileName.c_str(), info.fileSize, info.remoteFileName.c_str());
    }
    else if (info.status == firebase_fcs_upload_status_upload)
    {
        Serial.printf("Uploaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
    }
    else if (info.status == firebase_fcs_upload_status_complete)
    {
        Serial.println("Upload completed\n");
        FileMetaInfo meta = mAudioFbData.metaData();
        Serial.printf("Name: %s\n", meta.name.c_str());
        Serial.printf("Bucket: %s\n", meta.bucket.c_str());
        Serial.printf("contentType: %s\n", meta.contentType.c_str());
        Serial.printf("Size: %d\n", meta.size);
        Serial.printf("Generation: %lu\n", meta.generation);
        Serial.printf("Metageneration: %lu\n", meta.metageneration);
        Serial.printf("ETag: %s\n", meta.etag.c_str());
        Serial.printf("CRC32: %s\n", meta.crc32.c_str());
        Serial.printf("Tokens: %s\n", meta.downloadTokens.c_str());
        Serial.printf("Download URL: %s\n\n", mAudioFbData.downloadURL().c_str());
    }
    else if (info.status == firebase_fcs_upload_status_error)
    {
        Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
    }
}

void CFbClient::fcsDownloadCallback(FCS_DownloadStatusInfo info)
{
  if (info.status == firebase_fcs_download_status_init)
    {
        Serial.printf("Downloading file %s (%d) to %s\n", info.remoteFileName.c_str(), info.fileSize, info.localFileName.c_str());
    }
    else if (info.status == firebase_fcs_download_status_download)
    {
        Serial.printf("Downloaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
    }
    else if (info.status == firebase_fcs_download_status_complete)
    {
        Serial.println("Download completed\n");
    }
    else if (info.status == firebase_fcs_download_status_error)
    {
        Serial.printf("Download failed, %s\n", info.errorMsg.c_str());
    }
}
