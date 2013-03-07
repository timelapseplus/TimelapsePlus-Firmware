
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
#include <LUFA/Common/Common.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/Peripheral/ADC.h>

extern LCD lcd;
extern Clock clock;


Light::Light()
{
	// Configure I2C //
	TWI_Master_Initialise();
	lastSeconds = 0;
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
}

void Light::stop()
{
   unsigned char I2C_Buf[3];
   I2C_Buf[0] = I2C_ADDR_WRITE;
   I2C_Buf[1] = 0x00;
   I2C_Buf[2] = 0x00;
   TWI_Start_Read_Write(I2C_Buf, 3);
   lcd.backlight(255);
   lastSeconds = 0;
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
    lcd.backlight(0);
    _delay_ms(10);
    for(range = 0; range < 4; range++)
    {
        setRange(range);
        reading = readRaw();
        if(reading < 45000) break;
    }
}

int8_t Light::readEv()
{
    float lux;
    lux = readLux();
    lux *= 2^5;
    int8_t ev = (int8_t)ilog2(lux);
    ev *= 3;
    ev += 30;
    if(ev <= 11)
    {
    	uint16_t tmp = hardware_readLight(2) - 4;
    	if(tmp <= 8)
    	{
	    	ev = (int8_t)ilog2(tmp);
	    	ev *= 3;
	    	ev += 5;
    	}
    }
    return ev;
}

float Light::readIntegratedEv()
{
    uint16_t sum = 0;
    for(uint8_t i = 0; i < LIGHT_INTEGRATION_COUNT; i++) sum += iev[i];

    float value = (float)sum / (float)(LIGHT_INTEGRATION_COUNT);

    return value;
}

float Light::readIntegratedSlope()
{
    float value = ((float)readEv() - readIntegratedEv()) / (float)integration;

    value *= 30.0; // stops per 30min

    return value;
}

void Light::task()
{
	if(lastSeconds == 0) return;

    if(clock.Seconds() > lastSeconds + ((integration * 60) / LIGHT_INTEGRATION_COUNT))
    {
        lastSeconds = clock.Seconds();
        iev[pos] = readEv();
        pos++;
        if(pos >= LIGHT_INTEGRATION_COUNT) pos = 0;
        iev[pos] = 0;
    }
}

void Light::integrationStart(uint8_t integration_minutes)
{
	start();
    integration = integration_minutes;
    pos = 0;
    lastSeconds = clock.Seconds() + 1; // +1 so it can never be zero
    for(uint8_t i = 0; i < LIGHT_INTEGRATION_COUNT; i++)
    {
    	iev[i] = readEv(); // initialize array with readings //
    	wdt_reset();
    }
}

float Light::readLux()
{
    uint8_t range;
    uint16_t reading;
    float lux;
    lcd.backlight(0);
    for(range = 0; range < 4; range++)
    {
        setRange(range);
        reading = readRaw();
        if(reading < 45000) break;
    }

    if(range == 0)
    {
        for(uint8_t i = 0; i < 5; i++)
	    {
		    uint16_t reading2 = readRaw();
	        if(reading2 < reading) reading = reading2;
	    }	
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

