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
	//Timer for nightEv() updates
	nightSeconds = 0;						//J.R. 9-30-15
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

// removed to save space
//void Light::startIR()
//{
//    unsigned char I2C_Buf[3];
//    I2C_Buf[0] = I2C_ADDR_WRITE;
//    I2C_Buf[1] = 0x00;
//    I2C_Buf[2] = 0b11000000;
//    TWI_Start_Read_Write(I2C_Buf, 3);
//    I2C_Buf[0] = I2C_ADDR_WRITE;
//    I2C_Buf[1] = 0x01;
//    I2C_Buf[2] = 0b00000100;
//    TWI_Start_Read_Write(I2C_Buf, 3);
//    uint8_t range;
//    uint16_t reading;
//    _delay_ms(10);
//    for(range = 0; range < 4; range++)
//    {
//        I2C_Buf[0] = I2C_ADDR_WRITE;
//        I2C_Buf[1] = 0x01;
//        I2C_Buf[2] = range & 0b00000011;
//        TWI_Start_Read_Write(I2C_Buf, 3);
//        _delay_ms(10);
//        reading = readRaw();
//        if(reading < 45000) break;
//    }
//}

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
   //Timer for nightEv() updates
   nightSeconds = 0;						//J.R. 9-30-15
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

//This function is similar to "light.readIntegratedEv()" except that it calculates the 
//midpoint-median light value for the last 3 hours. 
//So "nightLight" is set wherever "lightReading" is set in shutter.cpp
float Light::readNightEv()														//J.R. 10-11-15
{
	if(!integrationActive) return 0.0;											//J.R. 10-11-15
	float value = arrayMedian50(nightEv, NIGHT_INTEGRATION_COUNT - 1);			//J.R. 10-11-15
	return value;																//J.R. 10-11-15
}

void Light::task()
{
	if(!initialized || !integrationActive) return;

  if(skipTask) return;

  if(paused)
  {
    lcd.backlight(255);
    wasPaused = 5;
    return;
  }

  if(wasPaused > 0)
  {
    lcd.backlight(0);
    wasPaused--;
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
    
    //The following 4 instructions were moved up so the "integrated" variable wold be available
	//for the lightThreshold (or NIGHT_THRESHOLD) comparison.
	median = arrayMedian50(iev, LIGHT_INTEGRATION_COUNT);						//J.R. 10-13-15

	float sum = 0.0;															//J.R. 10-13-15
	for(uint8_t i = 0; i < LIGHT_INTEGRATION_COUNT; i++) sum += iev[i];			//J.R. 10-13-15
	integrated = sum / (float)(LIGHT_INTEGRATION_COUNT);						//J.R. 10-13-15

    //No need to based the comparison on a single value of iev[LIGHT_INTEGRATION_COUNT]
	//since all light readings in the TL+ are based on the average of iev[], which is "integrated".
	//
	//Instead of comparing "integrated" to "NIGHT_THRESHOLD", "conf.lightThreshold" should be used,
	//which is the settable threshold value, rather than fixed.
	//
    //if(iev[LIGHT_INTEGRATION_COUNT - 1] <= NIGHT_THRESHOLD)
    if(integrated <= conf.lightThreshold)										//J.R. 10-13-15
    {
      underThreshold = true;
      if(lockedSlope == 0.0 && slope) lockedSlope = slope;
    }
    //else if(iev[LIGHT_INTEGRATION_COUNT - 1] > NIGHT_THRESHOLD + NIGHT_THRESHOLD_HYSTERESIS)
    else if(integrated > conf.lightThreshold + NIGHT_THRESHOLD_HYSTERESIS)	//J.R. 10-13-15
    {
      underThreshold = false;
      lockedSlope = 0.0;
    }


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
	//The same kind of loop is used here to update nightEv[] that was used for updating iev[].
	//Of course the time between updates is much longer. 
	if(nightSeconds == 0 || (clock.Seconds() > (nightSeconds + NIGHT_COUNT_DELAY)))	//J.R. 9-30-15
	{
		nightSeconds = clock.Seconds();  
			  
		for(uint8_t i = 0; i < NIGHT_INTEGRATION_COUNT - 1; i++)					//J.R. 9-30-15
		{
			nightEv[i] = nightEv[i + 1];//J.R. 9-30-15
		}
		//No need to do a readEv() all over again, as for iev[] above.
		nightEv[NIGHT_INTEGRATION_COUNT - 1] = iev[LIGHT_INTEGRATION_COUNT - 1];	//J.R. 9-30-15 
	} 
}

void Light::integrationStart(uint8_t integration_minutes)
{
	  start();
    //DEBUG(STR(" ####### LIGHT INTEGRATION START #######\r\n"));
    integration = (uint16_t)integration_minutes;
    lastSeconds = 0;
    //Timer for nightEv() updates
    nightSeconds = 0;						//J.R. 9-30-15
    for(uint8_t i = 0; i < LIGHT_INTEGRATION_COUNT; i++)
    {
    	iev[i] = readEv(); // initialize array with readings //
    	wdt_reset();
    }
    median = arrayMedian50(iev, LIGHT_INTEGRATION_COUNT);					//J.R. 9-30-15
	//This is where nightEv() is loaded with the current darkness level at the start:
	for(uint8_t i = 0; i < NIGHT_INTEGRATION_COUNT; i++)					//J.R. 9-30-15
	{
		nightEv[i] = median; // initialize night array with readings 		//J.R. 9-30-15;														//J.R. 9-30-15
	}
    integrationActive = true;
    slope = 0.0;
	//No need to set median and integrated to iev[0], since better values were obtained.
	//median = iev[0];														//J.R. 10-13-15
	//integrated = iev[0];													//J.R. 10-13-15
	integrated = median;													//J.R. 9-30-15
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
