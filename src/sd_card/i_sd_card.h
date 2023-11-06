/********************************************************************************************
 * SD card interface
 ********************************************************************************************/
#pragma once

class ISdCard
{
public:
  virtual ~ISdCard() = default;

  virtual bool setup() = 0;
  virtual int  read(uint8_t* oData, size_t iSize) = 0;
  virtual int  write(uint8_t* iData, size_t iSize) = 0;
};