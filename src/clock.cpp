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
#include "clock.h"
#include "button.h"
#include "5110LCD.h"
#include "hardware.h"
#include "settings.h"
#include "debug.h"


extern LCD lcd;
extern Button button;
extern settings conf;

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

volatile void Clock::count()
{
    ms++;
    event_ms++;

    if(ms >= 1000)
    {
        ms = 0;
        seconds++;
        sleep_time++;
        light_time++;
        flashlight_time++;
    }
    if(inTime > 0)
    {
        inTime--;
        if(inTime <= 0)
        {
            (*inFunction)();
        }
    }
    if(newJob)
    {
        if(jobStart) (*jobStart)();
        newJob = 0;
        jobRunning = 1;
    }
    else if(jobRunning)
    {
        jobDuration--;
        if(jobDuration <= 0)
        {
            if(jobComplete) (*jobComplete)();
            jobRunning = 0;
        }

    }
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
    jobRunning = 0;
    newJob = 0;
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
 *   Clock::job
 *
 *
 ******************************************************************/

void Clock::job(void (*startFunction)(), void (*endFunction)(), uint32_t duration)
{
    jobRunning = 1;
    jobStart = startFunction;
    jobComplete = endFunction;
    jobDuration = duration;
    newJob = 1;
}

/******************************************************************
 *
 *   Clock::in
 *
 *
 ******************************************************************/

void Clock::in(uint16_t stime, void (*func)())
{
    inFunction = func;   
    inTime = stime;
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

