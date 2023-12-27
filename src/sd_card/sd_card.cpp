/********************************************************************************************
 * RGB LED class
 ********************************************************************************************/
#include <HardwareSerial.h>

#include "sd_card.h"


#include "config/spi_config.h"

/********************************************************************************************
 * Initialize SPI and SD card
 * - run test sequence
 ********************************************************************************************/
bool CSdCard::setup()
{
  #if 0
  // default SPI of esp - matches my defines in SPI config - Regarding manual this pin selection is VSPI 
  Serial.print("MOSI: ");
  Serial.println(MOSI);
  Serial.print("MISO: ");
  Serial.println(MISO);
  Serial.print("SCK: ");
  Serial.println(SCK);
  Serial.print("SS: ");
  Serial.println(SS);
  
  // SPI is initialized automatically by using SD module
  // Lets initialize it on purpose, useful for later if we want to change ports
  // Use VSPI (General Purpose SPI1)
  //mSpi = new SPIClass(VSPI);
  //mSpi->begin(SPI_SD_CS, SPI_SD_MISO, SPI_SD_MOSI, SPI_SD_CS);
  //pinMode(SPI_SD_CS, OUTPUT);
  //digitalWrite(SPI_SD_CS, HIGH);
#endif

  // These are the default pins anyway
  SPI.begin(SPI_SD_SCLK, SPI_SD_MISO, SPI_SD_MOSI, SPI_SD_CS);

  if (!SD.begin(SPI_SD_CS))
  {
    Serial.println("Card Mount Failed");
    return false;
  }
  mCardMounted = true;

  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return false;;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC)
  {
    Serial.println("MMC");
  }
  else if(cardType == CARD_SD)
  {
    Serial.println("SDSC");
  }
  else if(cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
  }
  else
  {
    Serial.println("UNKNOWN");
    return false;
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // test sd card
  Serial.println("Test SD Card");
  test();
  return true;
}


bool CSdCard::open(bool iRead)
{
  mFile = SD.open("/recording.bin", (iRead ? FILE_READ : FILE_WRITE));
  
  if(!mFile)
  {
    Serial.println("Failed to open file");
    return false;
  }
  return true;
}


bool CSdCard::close()
{
  mFile.close();
  if(mFile)
  {
    Serial.println("Failed to close file");
    return false;
  }
  return true;
}

/********************************************************************************************
 * Read file from SD card
 * File must be openend manually before
 ********************************************************************************************/
size_t CSdCard::read(uint8_t *oData, size_t iSize, int iIdx)
{
  size_t rtrn = 0;

  if(!mFile)
  {
    Serial.println("Failed to open file for reading");
    return rtrn;
  }
  
  int available = mFile.available();
  //Serial.printf("File size: %u, available: %d\n", mFile.size(), available);

  if(available > 0)
  {
    rtrn = mFile.read(oData, iSize);
  }

  return rtrn;
}

/********************************************************************************************
 * Write file to SD card
 * File must be openend manually before
 ********************************************************************************************/
size_t CSdCard::write(uint8_t *iData, size_t iSize, int iIdx)
{
 size_t rtrn = 0;

  if(!mFile)
  {
    Serial.println("Failed to open file for writing");
    return rtrn;
  }

  rtrn = mFile.write(iData, iSize);

  if(rtrn == 0)
  {
    Serial.println("Write failed");
  }
  return rtrn;
}




/********************************************************************************************
 * SD card test
 ********************************************************************************************/
void CSdCard::test()
{
  list_dir(SD, "/", 0);
  create_dir(SD, "/mydir");
  list_dir(SD, "/", 0);
  remove_dir(SD, "/mydir");
  list_dir(SD, "/", 2);
  write_file(SD, "/hello.txt", "Hello ");
  append_file(SD, "/hello.txt", "World!\n");
  read_file(SD, "/hello.txt");
  delete_file(SD, "/foo.txt");
  rename_file(SD, "/hello.txt", "/foo.txt");
  read_file(SD, "/foo.txt");
  test_file_io(SD, "/test.txt");
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

/********************************************************************************************
 * List directories recursively
 ********************************************************************************************/
void CSdCard::list_dir(fs::FS &fs, const char * dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file)
  {
    if(file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels)
      {
        list_dir(fs, file.name(), levels -1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

/********************************************************************************************
 * Create directory on SD card
 ********************************************************************************************/
void CSdCard::create_dir(fs::FS &fs, const char * path)
{
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path))
  {
    Serial.println("Dir created");
  }
  else
  {
    Serial.println("mkdir failed");
  }
}

/********************************************************************************************
 * Remove directory from SD card
 ********************************************************************************************/
void CSdCard::remove_dir(fs::FS &fs, const char * path)
{
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path))
  {
    Serial.println("Dir removed");
  }
  else
  {
    Serial.println("rmdir failed");
  }
}

/********************************************************************************************
 * Read file from SD card and print
 ********************************************************************************************/
void CSdCard::read_file(fs::FS &fs, const char * path)
{
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available())
  {
    Serial.write(file.read());
  }
  file.close();
}

/********************************************************************************************
 * Write file and content to SD card
 ********************************************************************************************/
size_t CSdCard::write_file(fs::FS &fs, const char * path,  const uint8_t *buf, size_t size)
{
  size_t rtrn = 0;

  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file)
  {
    Serial.println("Failed to open file for writing");
    return rtrn;
  }

  rtrn = file.write(buf, size);

  if(rtrn > 0)
  {
    Serial.printf("File written - size: %u\n", file.size());
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();

  return rtrn;
}

/********************************************************************************************
 * Write file and content to SD card
 ********************************************************************************************/
void CSdCard::write_file(fs::FS &fs, const char * path, const char * message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message))
  {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}

/********************************************************************************************
 * Append text to file
 ********************************************************************************************/
void CSdCard::append_file(fs::FS &fs, const char * path, const char * message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message))
  {
    Serial.println("Message appended");
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}

/********************************************************************************************
 * Append text to file
 ********************************************************************************************/
size_t CSdCard::append_file(fs::FS &fs, const char * path, const uint8_t *buf, size_t size)
{
  size_t rtrn = 0;

  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file)
  {
    Serial.println("Failed to open file for appending");
    return rtrn;
  }

  rtrn = file.write(buf, size);

  if(rtrn > 0)
  {
    Serial.printf("File written - size: %u\n", file.size());
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();

  return rtrn;
}

/********************************************************************************************
 * Rename file
 ********************************************************************************************/
void CSdCard::rename_file(fs::FS &fs, const char * path1, const char * path2)
{
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2))
  {
    Serial.println("File renamed");
  }
  else
  {
    Serial.println("Rename failed");
  }
}

/********************************************************************************************
 * Delete file
 ********************************************************************************************/
void CSdCard::delete_file(fs::FS &fs, const char * path)
{
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path))
  {
    Serial.println("File deleted");
  }
  else
  {
    Serial.println("Delete failed");
  }
}

/********************************************************************************************
 * Dafuq
 ********************************************************************************************/
void CSdCard::test_file_io(fs::FS &fs, const char * path)
{
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file)
  {
    len = file.size();
    size_t flen = len;
    start = millis();
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  }
  else
  {
    Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if(!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for(i=0; i<2048; i++)
  {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}