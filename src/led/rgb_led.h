/********************************************************************************************
 * RGB LED class
 * - PWM on three GPIOs
 ********************************************************************************************/

#pragma once

#include "i_rgb_led.h"

#include <mutex>


class CRgbLed : public IRgbLed
{
public:
  CRgbLed() = default;
  ~CRgbLed() = default;

  void setup() override;
  void set_color(EColor iColor, int iValue) override;
  void set_state(bool iOn) override;

  int  get_color(EColor iColor) override;
  bool get_state() override;

private:
  bool mState     { false };
  int  mColors[3] { 0 /*BLUE*/, 0 /*GREEN*/, 0 /*RED*/ };

  std::recursive_mutex mMutex;
};