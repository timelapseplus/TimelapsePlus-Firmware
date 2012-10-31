/*
 *  shutter.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *	Licensed under GPLv3
 *
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/version.h>
#include <avr/eeprom.h>
#include <string.h>

#include "tldefs.h"
#include "shutter.h"
#include "clock.h"
#include "hardware.h"
#include "IR.h"
#include "debug.h"
#include "math.h"
#include "settings.h"
#include "timelapseplus.h"

#define DEBUG

#define RUN_DELAY 0
#define RUN_BULB 1
#define RUN_PHOTO 2
#define RUN_GAP 3
#define RUN_NEXT 4
#define RUN_END 5

extern IR ir;

extern settings conf;
extern shutter timer;

extern Clock clock;
volatile unsigned char state;
const uint16_t settings_warn_time = 0;
const uint16_t settings_mirror_up_time = 1;
volatile char cable_connected; // 1 = cable connected, 0 = disconnected

char shutter_state, ir_shutter_state; // used only for momentary toggle mode //

program stored[MAX_STORED]EEMEM;

/******************************************************************
 *
 *   shutter::
 *
 *
 ******************************************************************/

shutter::shutter()
{
    if(eeprom_read_byte((const uint8_t*)&stored[0].Name[0]) == 255) 
        setDefault();
    
    load(0); // Default Program //
    ENABLE_MIRROR;
    ENABLE_SHUTTER;
    CHECK_CABLE;
    shutter_state = 0;
    ir_shutter_state = 0;
}

/******************************************************************
 *
 *   shutter::setDefault
 *
 *
 ******************************************************************/

void shutter::setDefault()
{
    current.Name[0] = 'D';
    current.Name[1] = 'E';
    current.Name[2] = 'F';
    current.Name[3] = 'A';
    current.Name[4] = 'U';
    current.Name[5] = 'L';
    current.Name[6] = 'T';
    current.Name[7] = '\0';
    current.Delay = 5;
    current.Photos = 10;
    current.Gap = 20;
    current.Exps = 3;
    current.Mode = MODE_TIMELAPSE;
    current.Exp = 1;
    current.Bracket = 1;
    current.Keyframes = 1;
    save(0);
}

/******************************************************************
 *
 *   shutter::off
 *
 *
 ******************************************************************/
void shutter::off(void)
{
    shutter_off();
}
void shutter_off(void)
{
    if(conf.devMode) 
        hardware_flashlight(0);
    
    SHUTTER_CLOSE;
    MIRROR_DOWN; 
    clock.in(50, &check_cable);
    ir_shutter_state = 0;
    shutter_state = 0;
}

/******************************************************************
 *
 *   shutter::half
 *
 *
 ******************************************************************/

void shutter::half(void)
{
    shutter_half();
}
void shutter_half(void)
{
    if(conf.devMode) 
        hardware_flashlight(0);
    
    SHUTTER_CLOSE;
    MIRROR_UP;
}

/******************************************************************
 *
 *   shutter::full
 *
 *
 ******************************************************************/

void shutter::full(void)
{
    shutter_full();
}
void shutter_full(void)
{
    if(conf.devMode) 
        hardware_flashlight(1);
    
    MIRROR_UP;
    SHUTTER_OPEN;
}

/******************************************************************
 *
 *   shutter::bulbStart
 *
 *
 ******************************************************************/

void shutter::bulbStart(void)
{
    shutter_bulbStart();
}
void shutter_bulbStart(void)
{
    if(cable_connected == 0 && ir_shutter_state != 1)
    {
        ir_shutter_state = 1;
        ir.shutterNow();
//        ir.shutterDelayed();
    } 
    if(conf.bulbMode == 0)
    {
        shutter_full();
    } 
    else if(conf.bulbMode == 1 && shutter_state != 1)
    {
        shutter_full();
        shutter_state = 1;
        clock.in(75, &shutter_half);
    }
}

/******************************************************************
 *
 *   shutter::bulbEnd
 *
 *
 ******************************************************************/

void shutter::bulbEnd(void)
{
    shutter_bulbEnd();
}
void shutter_bulbEnd(void)
{
    if(cable_connected == 0 && ir_shutter_state == 1)
    {
        ir_shutter_state = 0;
        ir.shutterNow();
//        ir.shutterDelayed();
    } 
    if(conf.bulbMode == 0)
    {
        shutter_off();
    }
    else if(conf.bulbMode == 1 && shutter_state == 1)
    {
        shutter_full();
        shutter_state = 0;
        clock.in(75, &shutter_off);
    }
}

/******************************************************************
 *
 *   shutter::capture
 *
 *
 ******************************************************************/

void shutter::capture(void)
{
    if(cable_connected == 0)
    {
        ir.shutterNow();
//        ir.shutterDelayed();
    } 
    shutter_full();
    _delay_ms(75);
    shutter_off();
    ir_shutter_state = 0;
    shutter_state = 0;
}

/******************************************************************
 *
 *   shutter::cableIsConnected
 *
 *
 ******************************************************************/

char shutter::cableIsConnected(void)
{
    if(MIRROR_IS_DOWN)
    {
        CHECK_CABLE;
    }
    return cable_connected;
}

/******************************************************************
 *
 *   shutter::save
 *
 *
 ******************************************************************/

void shutter::save(char id)
{
    eeprom_write_block((const void*)&current, &stored[(uint8_t)id], sizeof(program));
    currentId = id;
}

/******************************************************************
 *
 *   shutter::load
 *
 *
 ******************************************************************/

void shutter::load(char id)
{
    eeprom_read_block((void*)&current, &stored[(uint8_t)id], sizeof(program));
    currentId = id;
}

/******************************************************************
 *
 *   shutter::nextId
 *
 *
 ******************************************************************/

int8_t shutter::nextId(void)
{
    int8_t id = -1, i;
    for(i = 1; i < MAX_STORED; i++)
    {
        if(eeprom_read_byte((uint8_t*)&stored[i].Name[0]) == 255)
        {
            id = i;
            break;
        }
    }
    return id;
}

/******************************************************************
 *
 *   shutter::begin
 *
 *
 ******************************************************************/

void shutter::begin()
{
    running = 1;
}

/******************************************************************
 *
 *   shutter::run
 *
 *
 ******************************************************************/

char shutter::task()
{
    char cancel = 0;
    static uint8_t enter, exps, run_state = RUN_DELAY, old_state = 255;
    static uint16_t photos;
    static uint32_t last_photo_ms, last_photo_end_ms;

    if(MIRROR_IS_DOWN)
    {
        CHECK_CABLE;
    }
    
    if(!running)
    {
        if(enter) 
            cancel = 1;
        else 
            return 0;
    }

//	uint16_t value;

    if(enter == 0) // Initialize variables and setup I/O pins
    {
        enter = 1;
        run_state = RUN_DELAY;
        clock.tare();
        photos = 0;
        exps = 0;
        current.infinitePhotos = current.Photos == 0 ? 1 : 0;
        status.infinitePhotos = current.infinitePhotos;
        status.photosRemaining = current.Photos;
        status.photosTaken = 0;
        last_photo_end_ms = 0;
        last_photo_ms = 0;

        ENABLE_MIRROR;
        ENABLE_SHUTTER;
        SHUTTER_CLOSE;
        MIRROR_DOWN;
    }

    /////// RUNNING PROCEDURE ///////
    if(run_state == RUN_DELAY)
    {
        if(old_state != run_state)
        {
            if(conf.devMode)
            {
                debug(STR("State: RUN_DELAY"));
                debug_nl();
            }
            strcpy((char *) status.textStatus, TEXT("Delay"));
            old_state = run_state;
        }


        if(((unsigned long)clock.event_ms / 1000) > current.Delay)
        {
            clock.tare();
            clock.reset();
            last_photo_ms = 0;
            run_state = RUN_PHOTO;
        } 
        else
        {
            status.nextPhoto = (unsigned int) (current.Delay - (unsigned long)clock.event_ms / 1000);
            if(((unsigned long)clock.event_ms / 1000) + settings_mirror_up_time >= current.Delay)
            {
                // Mirror Up //
                shutter_half();
            }

            if((settings_warn_time > 0) && (((unsigned long)clock.event_ms / 1000) + settings_warn_time >= current.Delay))
            {
                // Flash Light //
                _delay_ms(50);
            }
        }
    }

    if(run_state == RUN_PHOTO && (exps + photos == 0 || (uint8_t)((clock.Ms() - last_photo_end_ms) / 10) >= conf.cameraFPS))
    {
        last_photo_end_ms = 0;
        if(old_state != run_state)
        {
            if(conf.devMode)
            {
                debug(STR("State: RUN_PHOTO"));
                debug_nl();
            }
            strcpy((char *) status.textStatus, TEXT("Photo"));
            old_state = run_state;
        }
        if(current.Exp > 0 || (current.Mode & RAMP))
        {
            clock.tare();
            run_state = RUN_BULB;
        } 
        else
        {
            exps++;
            capture();
            
            if(current.Gap <= settings_mirror_up_time) 
                shutter_half(); // Mirror Up //

            run_state = RUN_NEXT;
        }
    }
    
    if(run_state == RUN_BULB)
    {
        static uint32_t bulb_length, exp;

        if(old_state != run_state)
        {
            old_state = run_state;

            if(conf.devMode)
            {
                debug(STR("State: RUN_BULB"));
                debug_nl();
            }
            strcpy((char *) status.textStatus, TEXT("Bulb"));

            exp = current.Exp * 100;

            if(current.Mode & RAMP)
            {
                float key1, key2, key3, key4;
                char found = 0;
                uint8_t i;

                // Bulb ramp algorithm goes here
                for(i = 1; i <= current.Keyframes; i++)
                {
                    if(clock.Seconds() <= current.Key[i - 1])
                    {
                        found = 1;
                        key1 = (float)current.Bulb[i > 1 ? i - 2 : i - 1] * 100;
                        key2 = (float)current.Bulb[i - 1] * 100;
                        key3 = (float)current.Bulb[i] * 100;
                        key4 = (float)current.Bulb[i < current.Keyframes ? i + 1 : i] * 100;
                        break;
                    }
                }
                
                if(found)
                {
                    exp = (uint32_t)curve(key1, key2, key3, key4, ((float)clock.Seconds() - (i > 1 ? (float)current.Key[i - 2] : 0.0)) / ((float)current.Key[i - 1] - (i > 1 ? (float)current.Key[i - 2] : 0.0)));
                } 
                else
                {
                    exp = current.Bulb[current.Keyframes] * 100;
                }
                
                bulb_length = exp;

                if(conf.devMode)
                {
                    debug(STR("Seconds: "));
                    debug((uint16_t)clock.Seconds());
                    debug_nl();
                    debug(STR("Ramp: "));
                    debug(bulb_length);

                    if(found) 
                        debug(STR(" (calculated)"));
                    
                    debug_nl();
                }
            }

            if(current.Mode & HDR)
            {
                uint32_t tmp = (exps - (current.Exps >> 1)) * current.Bracket;
                bulb_length = (tmp < (2^32/2)) ? exp * (1 << tmp) : exp / (1 << (0 - tmp));

                if(conf.devMode)
                {
                    debug(STR("exps - (current.Exps >> 1): "));

                    if(tmp < (2^32/2))
                    {
                        debug(tmp);
                    } 
                    else
                    {
                        debug(STR("-"));
                        debug(0 - tmp);
                    }
                    
                    debug_nl();
                    debug(STR("Bulb: "));
                    debug(bulb_length);
                    debug_nl();
                }
            }
            
            if((current.Mode & (HDR | RAMP)) == 0)
            {
                if(conf.devMode)
                {
                    debug(STR("***Using exp"));
                    debug_nl();
                }
                bulb_length = exp;
            }

            clock.job(&shutter_bulbStart, &shutter_bulbEnd, bulb_length);
        }
        else if(!clock.jobRunning)
        {
            exps++;

            _delay_ms(50);

            if(current.Gap <= settings_mirror_up_time) 
                shutter_half(); // Mirror Up //

            run_state = RUN_NEXT;
        }
    }
    
    if(run_state == RUN_NEXT)
    {
        last_photo_end_ms = clock.Ms();
        if(old_state != run_state)
        {
            if(conf.devMode)
            {
                debug(STR("State: RUN_NEXT"));
                debug_nl();
            }
            old_state = run_state;
        }

        if((exps >= current.Exps && (current.Mode & HDR)) || (current.Mode & HDR) == 0)
        {
            exps = 0;
            photos++;
            clock.tare();
            run_state = RUN_GAP;
        } 
        else
        {
            clock.tare();
            run_state = RUN_PHOTO;
        }
        
        if(current.infinitePhotos == 0)
        {
            if(photos >= current.Photos || (((current.Mode & TIMELAPSE) == 0) && photos >= 1))
            {
                run_state = RUN_END;
            }

            status.photosRemaining = current.Photos - photos;
            status.photosTaken = photos;
        }
        else
        {
            run_state = RUN_DELAY;
        }
    }
    
    if(run_state == RUN_GAP)
    {
        if(old_state != run_state)
        {
            if(conf.devMode)
            {
                debug(STR("State: RUN_GAP"));
                debug_nl();
            }
            strcpy((char *) status.textStatus, TEXT("Waiting"));
            old_state = run_state;
        }
        uint32_t cms = clock.Ms();

        if((cms - last_photo_ms) / 100 >= current.Gap)
        {
            last_photo_ms = cms;
            clock.tare();
            run_state = RUN_PHOTO;
        } 
        else
        {
            status.nextPhoto = (unsigned int) ((current.Gap - (cms - last_photo_ms) / 100) / 10);
            if((cms - last_photo_ms) / 100 + (uint32_t)settings_mirror_up_time * 10 >= current.Gap)
            {
                // Mirror Up //
                shutter_half();
            }
/*			if((settings_warn_time > 0) && ((cms - last_photo_ms) + (uint32_t)settings_warn_time * 1000 >= current.Gap * 100))
            {
                // Flash Light //
//				_delay_ms(50);
            }	
*/
        }
    }
    
    if(run_state == RUN_END)
    {
        if(old_state != run_state)
        {
            if(conf.devMode)
            {
                debug(STR("State: RUN_END"));
                debug_nl();
            }
            strcpy((char *) status.textStatus, TEXT("Done"));
            old_state = run_state;
        }

        enter = 0;
        running = 0;
        shutter_off();

        return DONE;
    }

    /////////////////////////////////////////

    if(cancel)
    {
        run_state = RUN_END;
    }

    //	Show_Number(clock.seconds);

    return CONTINUE;
}

void check_cable()
{
    CHECK_CABLE;
}



