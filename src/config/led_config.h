/**********************************************************************
 * RGB specific configurations
 **********************************************************************/
#pragma once

#include <map>
#include <WString.h>

/**********************************************************************
 * DEFINES
 **********************************************************************/
//#define PWM_GPIO_RED   GPIO_NUM_5
//#define PWM_GPIO_GREEN GPIO_NUM_18
//#define PWM_GPIO_BLUE  GPIO_NUM_19
// -> 5, 18, 19, 23 will be SD card SPI
#define PWM_GPIO_RED   GPIO_NUM_21
#define PWM_GPIO_GREEN GPIO_NUM_22
#define PWM_GPIO_BLUE  GPIO_NUM_4


/**********************************************************************
 * GLOBALs
 **********************************************************************/
enum EColor
{
    BLUE          = 0
,   GREEN         = 1
,   RED           = 2
,   NUM_OF_COLORS = 3
};

static std::map<EColor,String> sColorToString
{
    { EColor::BLUE,  "BLUE"  }
,   { EColor::GREEN, "GREEN" }
,   { EColor::RED,   "RED"   }
};
static std::map<String,EColor> sStringToColor
{
    { "BLUE",  EColor::BLUE  }
,   { "GREEN", EColor::GREEN }
,   { "RED",   EColor::RED   }
};