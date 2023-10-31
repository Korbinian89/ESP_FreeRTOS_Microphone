/********************************************************************************************
 * RGB LED class
 ********************************************************************************************/
#include <HardwareSerial.h>

#include "rgb_led.h"

#include "driver/gpio.h"

#include "config/led_config.h"

/********************************************************************************************
 * Initialize RGB LED GPIOs
 ********************************************************************************************/
void CRgbLed::setup()
{
  // Set the GPIO as a push/pull output
  gpio_reset_pin(PWM_GPIO_RED);
  gpio_set_direction(PWM_GPIO_RED, GPIO_MODE_OUTPUT);
  gpio_reset_pin(PWM_GPIO_GREEN);
  gpio_set_direction(PWM_GPIO_GREEN, GPIO_MODE_OUTPUT);
  gpio_reset_pin(PWM_GPIO_BLUE);
  gpio_set_direction(PWM_GPIO_BLUE, GPIO_MODE_OUTPUT);
}


/********************************************************************************************
 * Set value of a single color
 ********************************************************************************************/
void CRgbLed::set_color(EColor iColor, int iValue)
{
  std::lock_guard<std::recursive_mutex> lock(mMutex);

  Serial.printf("Set Color: %s to %d - state %s\n", sColorToString[iColor].c_str(), iValue, (mState ? "ON" : "OFF"));

  // update color
  mColors[iColor] = iValue;

  if (iColor == EColor::BLUE)
  {
    analogWrite(PWM_GPIO_BLUE, (mState) ? (iValue) : 0);
  }
  else if (iColor == EColor::GREEN)
  {
    analogWrite(PWM_GPIO_GREEN, (mState) ? (iValue) : 0);
  }
  else if (iColor == EColor::RED)
  {
    analogWrite(PWM_GPIO_RED, (mState) ? (iValue) : 0);
  }
  Serial.println();
}


/********************************************************************************************
 * Set state of all colors
 ********************************************************************************************/
void CRgbLed::set_state(bool iOn)
{
  std::lock_guard<std::recursive_mutex> lock(mMutex);

  // early return if state hasn't changed
  if (mState == iOn)
  {
    return;
  }

  printf("Set State to: %s\n", (iOn ? "ON" : "OFF"));

  mState = iOn;

  for (int i = 0; i < static_cast<int>(EColor::NUM_OF_COLORS); ++i)
  {
    set_color(EColor(i), mColors[i]);
  }
}


/********************************************************************************************
 * Get value for a given color
 ********************************************************************************************/
int CRgbLed::get_color(EColor iColor)
{
  std::lock_guard<std::recursive_mutex> lock(mMutex);
  return mColors[static_cast<int>(iColor)];
}


/********************************************************************************************
 * Get state of all colors
 ********************************************************************************************/
bool CRgbLed::get_state()
{
  std::lock_guard<std::recursive_mutex> lock(mMutex);
  return mState;
}