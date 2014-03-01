/*
 *  button.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */
 
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include "button.h"
#include "5110LCD.h"
#include "Menu.h"
#include "clock.h"
#include "hardware.h"
#include "tldefs.h"
// The followinging are interrupt-driven keypad reading functions
//  which includes DEBOUNCE ON/OFF mechanism, and continuous pressing detection

extern Clock clock;
extern MENU menu;

const unsigned char PROGMEM button_pins[] = { 4, 2, 4, 5, 7, 6 };

#define B_PORT PORTD
#define B_DDR DDRD
#define B_PIN PIND
#define FB_PORT PORTE
#define FB_DDR DDRE
#define FB_PIN PINE

/******************************************************************
 *
 *   Button::Button
 *
 *
 ******************************************************************/

Button::Button()
{
    // setup interrupt-driven keypad arrays
    // reset button arrays
    char p;
    
    for(uint8_t i = 0; i < NUM_KEYS; i++)
    {
        button_count[i] = 0;
        button_status[i] = 0;
        button_flag[i] = 0;
        p = pgm_read_byte(&button_pins[i]);
        
        if(i < 2)
        {
            clrBit(p, FB_DDR);
            setBit(p, FB_PORT);
        } 
        else
        {
            clrBit(p, B_DDR);
            setBit(p, B_PORT);
        }
    }
}

/******************************************************************
 *
 *   Button:poll
 *
 *
 ******************************************************************/

volatile void Button::poll()
{
    uint8_t i;
    char p;

    for(i = 0; i < NUM_KEYS; i++)
    {
        p = pgm_read_byte(&button_pins[i]);
        
        if(i < 2) 
            p = (getBit(p, FB_PIN) == LOW);
        else 
            p = (getBit(p, B_PIN) == LOW);
        
        if(p)  // key is pressed
        {
            if(button_count[i] < DEBOUNCE_REPEAT_DELAY)
            {
                button_count[i]++;
                
                if(button_count[i] > DEBOUNCE_ON)
                {
                    if(button_status[i] == 0)
                    {
                        button_flag[i] = 1;
                        button_status[i] = 1; //button debounced to 'pressed' status
                        clock.awake(); // keep from sleeping since a button was pressed
                    }

                }
            }
            else
            {
                if(i + 1 == UP_KEY || i + 1 == DOWN_KEY)
                {
                    button_flag[i] = 1;
                    button_status[i] = 1; //button debounced to 'pressed' status
                    button_count[i] = DEBOUNCE_REPEAT_DELAY - DEBOUNCE_REPEAT_SPEED;
                }
                else if(i + 1 == FL_KEY)
                {
                    off_count++;
                    if(off_count > POWER_OFF_TIME)
                    {
                        menu.message(TEXT("Power Off"));
                        menu.task();
                        cli();
                        while(getBit(p, FB_PIN) == LOW) wdt_reset();
                        hardware_off();
                    }
                }
            }

        } 
        else // no button pressed
        {
            if(i + 1 == FL_KEY) off_count = 0;
            if(button_count[i] > 0)
            {
                button_flag[i] = 0;
                if(button_count[i] > DEBOUNCE_MAX) button_count[i] = DEBOUNCE_MAX;
                button_count[i]--;
                
                if(button_count[i] < DEBOUNCE_OFF)
                {
                    button_status[i] = 0;   //button debounced to 'released' status
                }
            }
        }
    }
}

/******************************************************************
 *
 *   Button::get
 * 
 *   returns key pressed and removes it from the buffer
 *
 ******************************************************************/

char Button::get()
{
    char key;
    uint8_t i;

    if(clock.slept()) 
        flushBuffer();

    for(i = 0; i < NUM_KEYS; i++)
    {
        if(button_flag[i] != 0)
        {
            button_flag[i] = 0;  // reset button flag
            key = ++i;
            break;
        } 
        else
        {
            key = 0;
        }
    }
    
    return key;
}

/******************************************************************
 *
 *   Button::pressed
 * 
 *   returns key pressed and does not remove it from the buffer
 *
 ******************************************************************/

char Button::pressed()
{
    char key;
    uint8_t i;

    if(clock.slept()) 
        flushBuffer();
    
    for(i = 0; i < NUM_KEYS; i++)
    {
        if(button_flag[i] != 0)
        {
            key = ++i;
            break;
        } 
        else
        {
            key = 0;
        }
    }
    
    return key;
}

/******************************************************************
 *
 *   Button::waitfor
 *
 *
 ******************************************************************/

char Button::waitfor(char key)
{
    while (get() != key) 
        wdt_reset();
    
    return key;
}

/******************************************************************
 *
 *   Button::flushBuffer
 *
 *
 ******************************************************************/

void Button::flushBuffer()
{
    uint8_t i;

    for(i = 0; i < NUM_KEYS; i++)
    {
        button_count[i] = 0;
        button_status[i] = 0;
        button_flag[i] = 0;
    }
}

