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

#define CLOCK_IN_QUEUE 3

class Clock
{
public:
    Clock();

    void init();
    void disable();
    volatile void count();
    void advance(uint8_t advance_ms);
    void reset();
    void tare();
    void awake();
    void task();
    uint8_t slept();
    uint32_t eventMs();
    uint32_t event_ms;
    uint32_t seconds;
    uint16_t ms;
    uint32_t Seconds();
    uint32_t Ms();
    uint8_t sleepOk;
    uint8_t sleeping;

    uint8_t jobRunning;

    void job(void (*startFunction)(), void (*endFunction)(), uint32_t duration);
    void cancelJob(void);
    void in(uint16_t stime, void (*func)());

private:
    void (*jobStart)();
    void (*jobComplete)();
    uint32_t jobDuration;
    uint8_t newJob;

    uint8_t inIndex;
    uint16_t inTime[CLOCK_IN_QUEUE];
    void (*inFunction[CLOCK_IN_QUEUE])();

    uint8_t sleepWasOk;
    uint16_t light_time;
    uint16_t sleep_time;
    uint16_t flashlight_time;
    uint8_t wasSleeping;
    uint8_t backlightVal;
};

void wakeupFunction();
void Clock_count();
