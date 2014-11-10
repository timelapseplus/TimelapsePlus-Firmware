#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "hardware.h"
#include "button.h"
#include "5110LCD.h"
#include "Menu.h"
#include "clock.h"
#include "shutter.h"
#include "timelapseplus.h"
#include "tlp_menu_functions.h"
#include "tldefs.h"
#include "debug.h"
#include "bluetooth.h"
#include "TWI_Master.h"
#include "math.h"
#include "light.h"
#include "settings.h"
#include <LUFA/Common/Common.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/Peripheral/ADC.h>

extern LCD lcd;
extern Clock clock;
extern settings_t conf;

Light::Light()
{
	// Configure I2C //
	TWI_Master_Initialise();
	lastSeconds = 0;
  initialized = 0;
  integrationActive = false;
  lockedSlope = 0.0;
}

uint16_t Light::readRaw()
{
   unsigned char I2C_Buf[3];
   I2C_Buf[0] = I2C_ADDR_WRITE;
   I2C_Buf[1] = 0x02;
   TWI_Start_Read_Write(I2C_Buf, 2);
   _delay_us(1);
   I2C_Buf[0] = I2C_ADDR_READ;
   I2C_Buf[1] = 0;
   I2C_Buf[2] = 0;
   TWI_Start_Read_Write(I2C_Buf, 3);
   TWI_Read_Data_From_Buffer(I2C_Buf, 3);

   uint16_t result = I2C_Buf[1] + (I2C_Buf[2] << 8);
   return result;
}

void Light::startIR()
{
    unsigned char I2C_Buf[3];
    I2C_Buf[0] = I2C_ADDR_WRITE;
    I2C_Buf[1] = 0x00;
    I2C_Buf[2] = 0b11000000;
    TWI_Start_Read_Write(I2C_Buf, 3);
    I2C_Buf[0] = I2C_ADDR_WRITE;
    I2C_Buf[1] = 0x01;
    I2C_Buf[2] = 0b00000100;
    TWI_Start_Read_Write(I2C_Buf, 3);
    uint8_t range;
    uint16_t reading;
    _delay_ms(10);
    for(range = 0; range < 4; range++)
    {
        I2C_Buf[0] = I2C_ADDR_WRITE;
        I2C_Buf[1] = 0x01;
        I2C_Buf[2] = range & 0b00000011;
        TWI_Start_Read_Write(I2C_Buf, 3);
        _delay_ms(10);
        reading = readRaw();
        if(reading < 45000) break;
    }
}

void Light::start()
{
   unsigned char I2C_Buf[3];
   I2C_Buf[0] = I2C_ADDR_WRITE;
   I2C_Buf[1] = 0x00;
   I2C_Buf[2] = 0b10100000;
   TWI_Start_Read_Write(I2C_Buf, 3);
   offset = OFFSET_UNSET;
   filterIndex = -1;
   initialized = 1;
   paused = 0;
}

void Light::stop()
{
   unsigned char I2C_Buf[3];
   initialized = 0;
   I2C_Buf[0] = I2C_ADDR_WRITE;
   I2C_Buf[1] = 0x00;
   I2C_Buf[2] = 0x00;
   TWI_Start_Read_Write(I2C_Buf, 3);
   lcd.backlight(255);
   lastSeconds = 0;
   paused = 0;
   lockedSlope = 0.0;
   integrationActive = false;
}

void Light::setRange(uint8_t range)
{
   unsigned char I2C_Buf[3];
   I2C_Buf[0] = I2C_ADDR_WRITE;
   I2C_Buf[1] = 0x01;
   I2C_Buf[2] = range & 0b00000011;
   TWI_Start_Read_Write(I2C_Buf, 3);
   _delay_ms(10);
}

void Light::setRangeAuto()
{
    uint8_t range;
    uint16_t reading;
    if(!initialized || paused) return;
    lcd.backlight(0);
    _delay_ms(10);
    for(range = 0; range < 4; range++)
    {
        setRange(range);
        reading = readRaw();
        if(reading < 45000) break;
    }
}

float Light::readEv()
{
    float lux;
    static float lastReading;

    if(!initialized) return 0;

    if(paused || skipTask) return lastReading;

    lux = readLux();

	  method = 0;

    float ev = libc_log2(lux);

    ev *= 3;
    ev += 30;

    if(filterIndex < 0) // initialize buffer
    {
      for(uint8_t i = 0; i < FILTER_LENGTH; i++) filter[i] = ev;
      filterIndex = 0;
    }
    else // circular buffer
    {
      if(filterIndex < FILTER_LENGTH - 1) filterIndex++; else filterIndex = 0;
      filter[filterIndex] = ev;
    }

    ev = arrayMedian(filter, FILTER_LENGTH);

    lastReading = ev;

    return ev;
}

float Light::readIntegratedEv()
{
    if(!integrationActive) return 0.0;

    return integrated;
}

float Light::readIntegratedSlope()
{
    if(!integrationActive) return 0.0;

    float value = (readEv() - integrated) / (float)integration;

    value *= (60.0 / 3.0); // stops per hour

    return value;
}

float Light::readIntegratedSlopeMedian()
{
    if(!integrationActive) return 0.0;
    float slopes[LIGHT_INTEGRATION_COUNT - 1];
    for(uint8_t i = 0; i < LIGHT_INTEGRATION_COUNT - 1; i++)
    {
      slopes[i] = (iev[i] - iev[i + 1]) / ((float)integration / (float)LIGHT_INTEGRATION_COUNT);
    }

    float value = arrayMedian50(slopes, LIGHT_INTEGRATION_COUNT - 1);

    value *= (60.0 / 3.0); // stops per hour

    return value;
}

void Light::task()
{
	if(!initialized || !integrationActive) return;

  if(skipTask) return;

  if(paused)
  {
    lcd.backlight(255);
    wasPaused = 5;
    DEBUG_NL();
    return;
  }

  if(wasPaused > 0)
  {
    lcd.backlight(0);
    wasPaused--;
    DEBUG_NL();
    return;
  }


  if(lastSeconds == 0 || (clock.Seconds() > lastSeconds + (uint32_t)((integration * 60) / LIGHT_INTEGRATION_COUNT)))
  {
    lastSeconds = clock.Seconds();
    for(uint8_t i = 0; i < LIGHT_INTEGRATION_COUNT - 1; i++)
    {
      iev[i] = iev[i + 1];
    }
    iev[LIGHT_INTEGRATION_COUNT - 1] = readEv();
    slope = readIntegratedSlopeMedian();

    if(iev[LIGHT_INTEGRATION_COUNT - 1] <= NIGHT_THRESHOLD)
    {
      underThreshold = true;
      if(lockedSlope == 0.0 && slope) lockedSlope = slope;
    }
    else if(iev[LIGHT_INTEGRATION_COUNT - 1] > NIGHT_THRESHOLD + NIGHT_THRESHOLD_HYSTERESIS)
    {
      underThreshold = false;
      lockedSlope = 0.0;
    }

    median = arrayMedian50(iev, LIGHT_INTEGRATION_COUNT);

    float sum = 0.0;
    for(uint8_t i = 0; i < LIGHT_INTEGRATION_COUNT; i++) sum += iev[i];
    integrated = sum / (float)(LIGHT_INTEGRATION_COUNT);

    if(conf.debugEnabled)
    {
      //DEBUG(STR("\r\nIEV: "));
      //for(uint8_t i = 0; i < LIGHT_INTEGRATION_COUNT; i++)
      //{
      //  DEBUG(iev[i]);
      //  DEBUG(STR(","));
      //}
      //DEBUG_NL();
//
      //DEBUG(STR("#######LOCKED "));
      //DEBUG(lockedSlope);
      //DEBUG(STR(" #######\r\n"));
//
      //DEBUG(STR("####### SLOPE "));
      //DEBUG(slope);
      //DEBUG(STR(" #######\r\n"));
//
//
      //DEBUG(STR("#######   INT "));
      //DEBUG(integrated);
      //DEBUG(STR(" #######\r\n"));
//
      //DEBUG(STR("#######   MED "));
      //DEBUG(median);
      //DEBUG(STR(" #######\r\n"));
//
      //DEBUG(STR("#######    EV "));
      //DEBUG(iev[LIGHT_INTEGRATION_COUNT - 1]);
      //DEBUG(STR(" #######\r\n"));
    }
  }
}

void Light::integrationStart(uint8_t integration_minutes)
{
	  start();
    //DEBUG(STR(" ####### LIGHT INTEGRATION START #######\r\n"));
    integration = (uint16_t)integration_minutes;
    lastSeconds = 0;
    for(uint8_t i = 0; i < LIGHT_INTEGRATION_COUNT; i++)
    {
    	iev[i] = readEv(); // initialize array with readings //
    	wdt_reset();
    }
    integrationActive = true;
    slope = 0.0;
    median = iev[0];
    integrated = iev[0];
    lockedSlope = 0.0;
    task();
}

float Light::readLux()
{
    uint8_t range;
    uint16_t reading;
    float lux;
    if(!initialized || paused) return 0.0;
    lcd.backlight(0);
    for(range = 0; range < 4; range++)
    {
        setRange(range);
        reading = readRaw();
        if(reading < 45000) break;
    }

    scale = range;

    #define READINGS_COUNT 10
    uint32_t readings = 0;
    uint8_t count = 0;
    if(range == 0)
    {
      for(uint8_t i = 0; i < READINGS_COUNT; i++)
	    {
		    reading = readRaw();
        readings += reading;
        if(reading > 0) count++;
	    }
      if(count == 0) count = 1;
      reading = (uint16_t) (readings / count);
    }
	
    switch(range)
    {
        case 0:
            lux = (float)reading / 524.288;
            break;
        case 1:
            lux = (float)reading / 131.072;
            break;
        case 2:
            lux = (float)reading / 32.768;
            break;
        case 3:
        default:
            lux = (float)reading / 8.192;
            break;
    }
    return lux;
}
