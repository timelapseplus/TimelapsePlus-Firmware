/*
 *  button.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

#define FL_KEY 1
#define FR_KEY 2
#define LEFT_KEY 3
#define RIGHT_KEY 4
#define UP_KEY 5
#define DOWN_KEY 6

//keypad debounce parameter
#define DEBOUNCE_MAX 15
#define DEBOUNCE_REPEAT_DELAY 250
#define DEBOUNCE_REPEAT_SPEED 160
#define DEBOUNCE_ON  10
#define DEBOUNCE_OFF 3

#define NUM_KEYS 6


class Button
{
public:
    Button();
    volatile void poll();
    char get();
    char pressed();
    char waitfor(char key);
    void flushBuffer();

private:
    // debounce counters
    char button_count[NUM_KEYS];
    // button status - pressed/released
    char button_status[NUM_KEYS];
    // button on flags for user program
    char button_flag[NUM_KEYS];
};


