/********************************************************************************************
 * RGB LED interface
 * - PWM on three GPIOs
 ********************************************************************************************/
#pragma once

#include "../config/led_config.h"

class IRgbLed
{
public:
  virtual ~IRgbLed() = default;

  virtual void setup() = 0;
  virtual void set_color(EColor iColor, int iValue) = 0;
  virtual void set_state(bool iOn) = 0;

  virtual int  get_color(EColor iColor) = 0;
  virtual bool get_state() = 0;
};