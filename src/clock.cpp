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


Clock::Clock()
{
}

void Clock::init()
{
  // Clear on OCR2A match //
  TCCR2A = (1<<WGM21);
  TCCR2B = 0;

  TCCR2B |= (1<<CS22); // 64
  OCR2A = 122;//125; // Match value (for interrupt)

  // Normal clock source //
  ASSR = (0<<AS2);

  //Timer2 Compare Interrupt Enable  
  TIMSK2 = (1<<OCIE2A);

  wasSleeping = 0;
  sleepOk = 1;
  reset();

  sei();
}

void Clock::disable()
{
  TCCR2A = 0;
  TCCR2B = 0;
  OCR2A = 0;
  ASSR = 0;
  TIMSK2 = 0;
}

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
}

void Clock::reset()
{
    event_ms = 0;
    seconds = 0;
    ms = 0;
}

void Clock::tare()
{
    event_ms = 0;
}

void Clock::awake()
{
  sleep_time = 0;
  light_time = 0;
//  flashlight_time = 0;

  if(backlightVal)
  {
    lcd.backlight(backlightVal);
    backlightVal = 0;
  }
}

void Clock::sleep()
{
  if(!sleepOk) sleep_time = 0; else if(!sleepWasOk) awake();
  sleepWasOk = sleepOk;
  if(sleepOk && sleep_time >= (uint16_t) conf.sysOffTime * 10)
  {
//    lcd.off();
/*    attachInterrupt(1,wakeupFunction,LOW);
    sleep_mode();
    detachInterrupt(1);*/
    hardware_off();
    awake();
    wasSleeping = 1;
  }
  else if(backlightVal == 0 && light_time >= (uint16_t) conf.lcdBacklightTime * 10)
  {
    backlightVal = lcd.getBacklight();
    lcd.backlight(0);
  }
  if(hardware_flashlightIsOn())
  {
    if(flashlight_time >= (uint16_t) conf.flashlightOffTime * 10)
    {
      hardware_flashlight(0);
    }
  }
  else
  {
    flashlight_time = 0;
  }
}

uint32_t Clock::eventMs()
{
  return event_ms;
}

uint32_t Clock::Ms()
{
  return (uint32_t)ms + seconds * 1000;
}

uint32_t Clock::Seconds()
{
  return seconds;
}

char Clock::slept()
{
  char tmp;
  tmp = wasSleeping;
  wasSleeping = 0;
  return tmp;
}

void wakeupFunction(){
  lcd.init();
  lcd.update();
  return;
}


