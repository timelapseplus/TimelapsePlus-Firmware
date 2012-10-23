/*
 *  clock.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

#define CLOCK_TUNE 134
//#define CLOCK_TUNE 8

class Clock
{
public:
    Clock();

    void init();
    void disable();
    volatile void count();
    void reset();
    void tare();
    void awake();
    void sleep();
    char slept();
    uint32_t eventMs();
    uint32_t event_ms;
    uint32_t seconds;
    uint16_t ms;
    uint32_t Seconds();
    uint32_t Ms();
    char sleepOk;
    char sleeping;

private:
    char sleepWasOk;
    uint16_t light_time;
    uint16_t sleep_time;
    uint16_t flashlight_time;
    char wasSleeping;
    unsigned char backlightVal;
};

void wakeupFunction();
