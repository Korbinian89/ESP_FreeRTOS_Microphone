/********************************************************************************************
 * SD card convenience class
 ********************************************************************************************/

#pragma once

#include "i_sd_card.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

class CSdCard : public ISdCard
{
public:
  CSdCard() = default;
  ~CSdCard() = default;

  bool setup() override;
  int  read(uint8_t* oData, size_t iSize) override;
  int  write(uint8_t* iData, size_t iSize) override;

private:

  SPIClass* mSpi         { nullptr };
  bool      mCardMounted { false   };

  void test();
  void list_dir(fs::FS &fs, const char * dirname, uint8_t levels);
  void create_dir(fs::FS &fs, const char * path);
  void remove_dir(fs::FS &fs, const char * path);
  void read_file(fs::FS &fs, const char * path);
  void write_file(fs::FS &fs, const char * path, const char * message);
  void append_file(fs::FS &fs, const char * path, const char * message);
  void rename_file(fs::FS &fs, const char * path1, const char * path2);
  void delete_file(fs::FS &fs, const char * path);
  void test_file_io(fs::FS &fs, const char * path);
};