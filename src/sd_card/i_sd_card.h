/********************************************************************************************
 * SD card interface
 ********************************************************************************************/
#pragma once

class SPIClass;

class ISdCard
{
public:
  virtual ~ISdCard() = default;

  virtual bool   setup() = 0;
  virtual bool   open(bool iRead) = 0;
  virtual bool   close() = 0;
  virtual size_t read(uint8_t* oData, size_t iSize, int iIdx) = 0;
  virtual size_t write(uint8_t* iData, size_t iSize, int iIdx) = 0;
  virtual bool   delete_recording_download() = 0;
  virtual SPIClass* get_spi() = 0;
};