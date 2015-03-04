/*
 *  clock.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */
 
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "clock.h"
#include "button.h"
#include "5110LCD.h"
#include "hardware.h"
#include "shutter.h"
#include "settings.h"
#include "debug.h"
#include "light.h"


extern LCD lcd;
extern Button button;
extern settings_t conf;
extern Light light;

/******************************************************************
 *
 *   Clock::Clock
 *
 *
 ******************************************************************/

Clock::Clock()
{
}

/******************************************************************
 *
 *   Clock::init
 *
 *
 ******************************************************************/

void Clock::init()
{
    // Clear on OCR2A match //
    TCCR2A = (1 << WGM21);
    TCCR2B = 0;

    TCCR2B |= (1 << CS22); // 64
    OCR2A = 122; //125; // Match value (for interrupt)

    // Normal clock source //
    ASSR = (0 << AS2);

    //Timer2 Compare Interrupt Enable
    TIMSK2 = (1 << OCIE2A);

    wasSleeping = 0;
    sleepOk = 1;
    sleeping = 0;
    reset();

    sei();
}

/******************************************************************
 *
 *   Clock::disable
 *
 *
 ******************************************************************/

void Clock::disable()
{
    TCCR2A = 0;
    TCCR2B = 0;
    OCR2A = 0;
    ASSR = 0;
    TIMSK2 = 0;
}

/******************************************************************
 *
 *   Clock::count
 *
 *
 ******************************************************************/
uint8_t skips = 0;
volatile void Clock::count()
{
    ms++;
    event_ms++;

    if(newBulb)
    {
        if(conf.auxPort == AUX_MODE_SYNC && !AUX_INPUT1)
        {
            if(bulbDuration > conf.camera.bulbEndOffset) bulbDurationPCsync = bulbDuration - conf.camera.bulbEndOffset; else bulbDurationPCsync = 0;
            if(conf.pcSyncRequired) bulbDuration += 2000; // wait up to 2 seconds for pc-sync input
        }
        else
        {
            bulbDurationPCsync = 0;
        }
        shutter_bulbStart();
        newBulb = 0;
        bulbRunning = 1;
        if(conf.camera.negBulbOffset)
        {
            bulbDuration -= conf.camera.bulbOffset;
        }
        else
        {
            bulbDuration += conf.camera.bulbOffset;
        }
        skips++;
        return;
    }
    else if(bulbRunning)
    {
        bulbDuration--;
        if(bulbDuration <= 0)
        {
            shutter_bulbEnd();
            bulbRunning = 0;
            light.skipTask = 0;
            if(bulbDurationPCsync) usingSync = 0;
            skips++;
            return;
        }
        else if(bulbDurationPCsync && AUX_INPUT1)
        {
            bulbDuration = bulbDurationPCsync;
            bulbDurationPCsync = 0;
            usingSync = 1;
            skips++;
            return;
        }
    }

    for(uint8_t i = 0; i < CLOCK_IN_QUEUE; i++)
    {
        if(inTime[i] > 0)
        {
            inTime[i] -= 1 + skips;
            if(inTime[i] <= 0)
            {
                (*inFunction[i])();
            }
        }
    }

    if(ms >= 1000)
    {
        ms -= 1000;
        seconds++;
        sleep_time++;
        light_time++;
        flashlight_time++;
    }
    skips = 0;
}

/******************************************************************
 *
 *   Clock::advance
 *   Only call with interrupts disabled!
 *
 ******************************************************************/

void Clock::advance(uint8_t advance_ms)
{
    while(advance_ms--) count();
}

/******************************************************************
 *
 *   Clock::reset
 *
 *
 ******************************************************************/

void Clock::reset()
{
    event_ms = 0;
    seconds = 0;
    ms = 0;
    bulbRunning = 0;
    newBulb = 0;
    usingSync = 0;
}

/******************************************************************
 *
 *   Clock::tare
 *
 *
 ******************************************************************/

void Clock::tare()
{
    event_ms = 0;
}

/******************************************************************
 *
 *   Clock::awake
 *
 *
 ******************************************************************/

void Clock::awake()
{
    sleeping = 0;
    sleep_time = 0;
    light_time = 0;
//  flashlight_time = 0;

    if(backlightVal)
    {
        lcd.backlight(backlightVal);
        backlightVal = 0;
    }
}

/******************************************************************
 *
 *   Clock::sleep
 *
 *
 ******************************************************************/

void Clock::task()
{
    if(!sleepOk)
    {
        sleep_time = 0;
    }
    else
    {
        if(!sleepWasOk) 
            awake();
    }
    
    sleepWasOk = sleepOk;
    
    if(!sleeping && sleepOk && sleep_time >= (uint16_t)conf.sysOffTime * 10)
    {
        sleeping = 1;
//    lcd.off();
/*    attachInterrupt(1,wakeupFunction,LOW);
    sleep_mode();
    detachInterrupt(1);*/
        hardware_off();
//        awake();
//        wasSleeping = 1;

    } 
    else if(backlightVal == 0 && light_time >= (uint16_t)conf.lcdBacklightTime * 10)
    {
        backlightVal = lcd.getBacklight();
        lcd.backlight(0);
    }
    
    if(hardware_flashlightIsOn())
    {
        if(flashlight_time >= (uint16_t)conf.flashlightOffTime * 10)
        {
            hardware_flashlight(0);
        }
        return;
    } 

    flashlight_time = 0;
}

/******************************************************************
 *
 *   Clock::eventMs
 *
 *
 ******************************************************************/

uint32_t Clock::eventMs()
{
    return event_ms;
}

/******************************************************************
 *
 *   Clock::Ms
 *
 *
 ******************************************************************/

uint32_t Clock::Ms()
{
    return (uint32_t)ms + seconds * 1000;
}

/******************************************************************
 *
 *   Clock::Seconds
 *
 *
 ******************************************************************/

uint32_t Clock::Seconds()
{
    return seconds;
}

/******************************************************************
 *
 *   Clock::slept
 *
 *
 ******************************************************************/

uint8_t Clock::slept()
{
    char tmp;
    
    tmp = wasSleeping;
    wasSleeping = 0;
    
    return tmp;
}

/******************************************************************
 *
 *   Clock::bulb
 *
 *
 ******************************************************************/

void Clock::bulb(uint32_t duration)
{
    if(conf.auxPort == AUX_MODE_SYNC) ENABLE_AUX_PORT1;
    light.skipTask = 1; // don't read I2C during timing
    bulbRunning = 0;
    bulbDuration = duration;
    newBulb = 1;
}

/******************************************************************
 *
 *   Clock::cancelBulb
 *
 *
 ******************************************************************/

void Clock::cancelBulb()
{
    light.skipTask = 0;
    bulbRunning = 0;
    bulbDuration = 0;
    newBulb = 0;
}

/******************************************************************
 *
 *   Clock::in
 *
 *
 ******************************************************************/

void Clock::in(uint16_t stime, void (*func)())
{
    for(uint8_t i = 0; i < CLOCK_IN_QUEUE; i++)
    {
        if(inTime[i] <= 0)
        {
            inFunction[i] = func;
            inTime[i] = stime;
            break;
        }
    }
}

/******************************************************************
 *
 *   Clock::wakeupFunction
 *
 *
 ******************************************************************/

void wakeupFunction()
{
    lcd.init(conf.lcdContrast);
    lcd.update();
    
    return;
}

