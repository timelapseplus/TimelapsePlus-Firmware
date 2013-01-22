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
#include "bluetooth.h"
#include "debug.h"
#include "math.h"
#include "settings.h"
#include "PTP_Driver.h"
#include "PTP.h"
#include "timelapseplus.h"

#define DEBUG

#define RUN_DELAY 0
#define RUN_BULB 1
#define RUN_PHOTO 2
#define RUN_GAP 3
#define RUN_NEXT 4
#define RUN_END 5
#define RUN_ERROR 6

extern IR ir;
extern PTP camera;
extern settings conf;
extern shutter timer;
extern BT bt;

extern Clock clock;
volatile unsigned char state;
const uint16_t settings_warn_time = 0;
const uint16_t settings_mirror_up_time = 1;
volatile char cable_connected; // 1 = cable connected, 0 = disconnected

char shutter_state, ir_shutter_state; // used only for momentary toggle mode //
uint16_t BulbMax; // calculated during bulb ramp mode
program stored[MAX_STORED]EEMEM;
uint8_t BulbMaxEv;

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
    current.Exp = 40;
    current.Bracket = 6;
    current.Keyframes = 1;
    current.Duration = 3600;
    current.BulbStart = 47;
    current.Bulb[0] = 36;
    current.Key[0] = 3600;
    save(0);

    uint8_t i;
    for(i = 1; i < MAX_STORED; i++)
    {
        eeprom_write_byte((uint8_t*)&stored[i].Name[0],  255);
    }
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
    clock.in(20, &check_cable);
    ir_shutter_state = 0;
    shutter_state = 0;
}
void shutter_off_quick(void)
{
    SHUTTER_CLOSE;
    MIRROR_DOWN;
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
    shutter_off(); // first we completely release the shutter button since some cameras need this to release the bulb
    if(conf.halfPress == HALF_PRESS_ENABLED) clock.in(30, &shutter_half_delayed);
}
void shutter_half_delayed(void)
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
        if(camera.supports.bulb)
        {
            ir_shutter_state = 1;
            camera.bulbStart();
        }
        else
        {
            ir_shutter_state = 1;
            ir.bulbStart();
        }
    } 
    if(conf.bulbMode == 0)
    {
        shutter_full();
    } 
    else if(conf.bulbMode == 1 && shutter_state != 1)
    {
        shutter_full();
        shutter_state = 1;
        clock.in(SHUTTER_PRESS_TIME, &shutter_half);
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
        if(camera.supports.bulb)
        {
            ir_shutter_state = 0;
            camera.bulbEnd();
        }
        else
        {
            ir_shutter_state = 0;
            ir.bulbEnd();
        }
    } 
    if(conf.bulbMode == 0)
    {
        shutter_off();
    }
    else if(conf.bulbMode == 1 && shutter_state == 1)
    {
        shutter_full();
        shutter_state = 0;
        clock.in(SHUTTER_PRESS_TIME, &shutter_off);
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
    shutter_capture();
}
void shutter_capture(void)
{
    shutter_full();
    clock.in(SHUTTER_PRESS_TIME, &shutter_off);
    ir_shutter_state = 0;
    shutter_state = 0;
    if(cable_connected == 0)
    {
        if(camera.supports.capture)
        {
            camera.capture();
        }
        else
        {
            ir.shutterNow();
        }
    } 
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
    static int8_t evShift;
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

    if(enter == 0) // Initialize variables and setup I/O pins
    {
        enter = 1;
        run_state = RUN_DELAY;
        clock.tare();
        photos = 0;
        exps = 0;
        if(current.Mode & RAMP)
        {
            current.Photos = (current.Duration * 10) / current.Gap;
            calcBulbMax();
        }
        if(conf.devMode)
        {
            debug(STR("Photos: "));
            debug(current.Photos);
            debug_nl();
        }
        current.infinitePhotos = current.Photos == 0 ? 1 : 0;
        status.infinitePhotos = current.infinitePhotos;
        status.photosRemaining = current.Photos;
        status.photosTaken = 0;
        status.mode = (uint8_t) current.Mode;
        last_photo_end_ms = 0;
        last_photo_ms = 0;
        evShift = 0;

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
        static uint8_t m = SHUTTER_MODE_BULB;

        if(old_state != run_state)
        {
            old_state = run_state;

            if(conf.devMode)
            {
                debug(STR("State: RUN_BULB"));
                debug_nl();
            }
            strcpy((char *) status.textStatus, TEXT("Bulb"));

            exp = camera.bulbTime((int8_t)current.Exp);
            m = camera.shutterType(current.Exp);

            if(current.Mode & RAMP)
            {
                float key1 = 1, key2 = 1, key3 = 1, key4 = 1;
                char found = 0;
                uint8_t i;
                m = SHUTTER_MODE_BULB;
                shutter_off();

                // Bulb ramp algorithm goes here
                for(i = 0; i <= current.Keyframes; i++)
                {
                    if(clock.Seconds() <= current.Key[i])
                    {
                        found = 1;
                        if(i == 0)
                        {
                            key2 = key1 = (float)(current.BulbStart);
                        }
                        else if(i == 1)
                        {
                            key1 = (float)(current.BulbStart);
                            key2 = (float)((int8_t)current.BulbStart - *((int8_t*)&current.Bulb[i - 1]));
                        }
                        else
                        {
                            key1 = (float)((int8_t)current.BulbStart - *((int8_t*)&current.Bulb[i - 2]));
                            key2 = (float)((int8_t)current.BulbStart - *((int8_t*)&current.Bulb[i - 1]));
                        }
                        key3 = (float)((int8_t)current.BulbStart - *((int8_t*)&current.Bulb[i]));
                        key4 = (float)((int8_t)current.BulbStart - *((int8_t*)&current.Bulb[i < (current.Keyframes - 1) ? i + 1 : i]));
                        break;
                    }
                }
                
                if(found)
                {
                    uint32_t var1 = clock.Seconds();
                    uint32_t var2 = (i > 0 ? current.Key[i - 1] : 0);
                    uint32_t var3 = current.Key[i];

                    bt.send(STR("KEY1: "));
                    sendHex((char *) &key1);
                    bt.send(STR("KEY2: "));
                    sendHex((char *) &key2);
                    bt.send(STR("KEY3: "));
                    sendHex((char *) &key3);
                    bt.send(STR("KEY4: "));
                    sendHex((char *) &key4);

                    float t = (float)(var1 - var2) / (float)(var3 - var2);
                    exp = camera.bulbTime(curve(key1, key2, key3, key4, t) - (float)evShift);
                }
                else
                {
                    exp = camera.bulbTime((int8_t)(current.BulbStart - *((int8_t*)&current.Bulb[current.Keyframes - 1]) - (float)evShift));
                }

                bulb_length = exp;

                if(camera.supports.iso)
                {
                    uint8_t iso = camera.iso();
                    uint8_t nextISO = iso;
                    int8_t tmpShift = 0;

                    // Check for too long bulb time //
                    while(bulb_length > BulbMax)
                    {
                        nextISO = camera.isoUp(iso);
                        if(nextISO != iso)
                        {
                            evShift += nextISO - iso;
                            tmpShift += nextISO - iso;
                            iso = nextISO;

                            bt.send(STR("   ISO UP:"));
                            sendByte((uint8_t)evShift);
                            bt.send(STR("   ISO Val:"));
                            sendByte((uint8_t)nextISO);
                        }
                        else
                        {
                            bt.send(STR("   Reached ISO Max!!!\r\n"));
                            break;
                        }
                        bt.send(STR("   Done!\r\n\r\n"));
                        bulb_length = camera.shiftBulb(exp, tmpShift);
                    }
                    bt.send(STR("Bulb Length (exp): "));
                    sendHex((char *) &exp);
                    bt.send(STR("Bulb Length (adjusted): "));
                    sendHex((char *) &bulb_length);

                    // Check for too short bulb time //
                    while(bulb_length < 99)
                    {
                        nextISO = camera.isoDown(iso);
                        if(nextISO != iso)
                        {
                            evShift -= iso - nextISO;
                            tmpShift -= iso - nextISO;

                            bt.send(STR("   ISO DOWN:"));
                            sendByte((uint8_t)evShift);
                            bt.send(STR("   ISO Val:"));
                            sendByte((uint8_t)nextISO);
                        }
                        else
                        {
                            bt.send(STR("   Reached ISO Min!!!\r\n"));
                            break;
                        }
                        bt.send(STR("   Done!\r\n\r\n"));
                        bulb_length = camera.shiftBulb(exp, tmpShift);
                    }

                    // Change the ISO //
                    shutter_off();
                    if(camera.iso() != nextISO)
                    {
                        if(camera.setISO(nextISO) == PTP_RETURN_ERROR)
                        {
                            run_state = RUN_ERROR;
                            return CONTINUE;
                        }
                    }
                    shutter_half();

                }
                
                if(conf.devMode)
                {
                    debug(STR("Seconds: "));
                    debug((uint16_t)clock.Seconds());
                    debug_nl();
                    debug(STR("BulbStart: "));
                    debug((uint16_t)camera.bulbTime((int8_t)current.BulbStart));
                    debug_nl();
                    debug(STR("Ramp: "));
                    debug((uint16_t)bulb_length);

                    if(found) 
                        debug(STR(" (calculated)"));
                    
                    debug_nl();


                    debug(STR("i: "));
                    debug(current.Bulb[i]);
                    debug_nl();
                    debug(STR("Key1: "));
                    debug((uint16_t)key1);
                    debug_nl();
                    debug(STR("Key2: "));
                    debug((uint16_t)key2);
                    debug_nl();
                    debug(STR("Key3: "));
                    debug((uint16_t)key3);
                    debug_nl();
                    debug(STR("Key4: "));
                    debug((uint16_t)key4);
                    debug_nl();
                }
            }

            if(current.Mode & HDR)
            {
                //uint32_t tmp = (exps - (current.Exps >> 1)) * current.Bracket;
                //bulb_length = (tmp < (2^32/2)) ? exp * (1 << tmp) : exp / (1 << (0 - tmp));

                uint8_t tv_offset = ((current.Exps - 1) / 2) * current.Bracket - exps * current.Bracket;
                if(current.Mode & RAMP)
                {
                    bulb_length = camera.shiftBulb(bulb_length, tv_offset);
                }
                else
                {
                    m = camera.shutterType(current.Exp - tv_offset);
                    if(m == 0) m = SHUTTER_MODE_PTP;

                    if(m & SHUTTER_MODE_PTP)
                    {
                        bt.send(STR("Shutter Mode PTP\r\n"));
                        camera.setShutter(current.Exp - tv_offset);
                        bulb_length = camera.bulbTime((int8_t)(current.Exp - tv_offset));
                    }
                    else
                    {
                        bt.send(STR("Shutter Mode BULB\r\n"));
                        m = SHUTTER_MODE_BULB;
                        bulb_length = camera.bulbTime((int8_t)(current.Exp - tv_offset));
                    }
                }

                if(conf.devMode)
                {
                    debug_nl();
                    debug(STR("Mode: "));
                    debug(m);
                    debug_nl();
                    debug(STR("Tv: "));
                    debug(current.Exp - tv_offset);
                    debug_nl();
                    debug(STR("Bulb: "));
                    debug((uint16_t)bulb_length);
                    debug_nl();
                }
            }
            
            if((current.Mode & (HDR | RAMP)) == 0)
            {
                if(conf.devMode)
                {
                    debug(STR("***Using exp: "));
                    debug(exp);
                    debug(STR(" ("));
                    debug(current.Exp);
                    debug(STR(")"));
                    debug_nl();
                }
                bulb_length = exp;
                if(m & SHUTTER_MODE_PTP)
                {
                    camera.setShutter(current.Exp);
                }
            }

            if(m & SHUTTER_MODE_PTP)
            {
                camera.capture();
                //clock.job(0, 0, bulb_length);
            }
            else
            {
                clock.job(&shutter_bulbStart, &shutter_bulbEnd, bulb_length + conf.bulbOffset);
            }
        }
        else if(!clock.jobRunning && !camera.busy)
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
        }
        else
        {
            run_state = RUN_GAP;
        }

        status.photosRemaining = current.Photos - photos;
        status.photosTaken = photos;
    }
    
    if(run_state == RUN_GAP)
    {
        if(old_state != run_state)
        {
            if(run_state == RUN_GAP && conf.auxPort == AUX_MODE_DOLLY) aux_pulse();
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
    
    if(run_state == RUN_ERROR)
    {
        if(old_state != run_state)
        {
            if(conf.devMode)
            {
                debug(STR("State: RUN_ERROR"));
                debug_nl();
            }
            strcpy((char *) status.textStatus, TEXT("Error"));
            old_state = run_state;
        }

        enter = 0;
        //running = 0;
        shutter_off();
        camera.bulbEnd();

        hardware_flashlight((uint8_t) clock.Seconds() % 2);

        return RUN_ERROR;
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
        camera.bulbEnd();
        hardware_flashlight(0);

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

void aux_pulse()
{
    aux_on();
    clock.in(100, &aux_off);
}

void aux_on()
{
    ENABLE_AUX_PORT;
    AUX_OUT1_ON;
    AUX_OUT2_ON;
}

void aux_off()
{
    ENABLE_AUX_PORT;
}

uint8_t stopName(char name[8], uint8_t stop)
{
    name[0] = ' ';
    name[1] = ' ';
    name[2] = ' ';
    name[3] = ' ';
    name[4] = ' ';
    name[5] = ' ';
    name[6] = ' ';
    name[7] = '\0';

    int8_t ev = *((int8_t*)&stop);

    uint8_t sign = 0;
    if(ev < 0)
    {
        sign = 1;
        ev = 0 - ev;
    }
    uint8_t mod = ev % 3;
    ev /= 3;
    if(mod == 0)
    {
        if(sign) name[5] = '-'; else name[5] = '+';
        if(ev > 9)
        {
            name[4] = name[5];
            name[5] = '0' + ev / 10;
            ev %= 10;
            name[6] = '0' + ev;
        }
        else
        {
            name[6] = '0' + ev;
        }
    }
    else
    {
        if(sign) name[ev > 0 ? 1 : 3] = '-'; else name[ev > 0 ? 1 : 3] = '+';
        if(ev > 9)
        {
            name[0] = name[1];
            name[1] = '0' + ev / 10;
            ev %= 10;
            name[2] = '0' + ev;
        }
        else if(ev > 0)
        {
            name[2] = '0' + ev;
        }
        name[4] = '0' + mod;
        name[5] = '/';
        name[6] = '3';
    }
    return 1;
}

void calcBulbMax()
{
    BulbMax = (timer.current.Gap / 10 - 5) * 1000;
    BulbMaxEv = 1;
    for(uint8_t i = camera.bulbMax(); i < camera.bulbMin(); i++)
    {
        if(BulbMax > camera.bulbTime((int8_t)i))
        {
            if(i < 1) i = 1;
            BulbMaxEv = i - 1;
            break;
        }
    }

    BulbMax = camera.bulbTime((int8_t)BulbMaxEv);
}

uint8_t stopUp(uint8_t stop)
{
    int8_t ev = *((int8_t*)&stop);

    calcBulbMax();

    if(ev < 25*3) ev++; else ev = 25*3;

    int8_t evMax = ((int8_t)camera.iso() - (int8_t)camera.isoMax()) + ((int8_t)timer.current.BulbStart - (int8_t)BulbMaxEv);

    if(ev <= evMax) return ev; else return evMax;

    return ev;
}

uint8_t stopDown(uint8_t stop)
{
    int8_t ev = *((int8_t*)&stop);

    if(ev > -25*3) ev--; else ev = -25*3;

    int8_t evMin = 0 - ((int8_t)camera.isoMin() - (int8_t)camera.iso() + (camera.bulbMin() - (int8_t)timer.current.BulbStart));

    if(ev >= evMin) return ev; else return stop;

    return ev;
}

uint8_t checkHDR(uint8_t exps, uint8_t mid, uint8_t bracket)
{
    uint8_t up = mid - (exps / 2) * bracket;
    uint8_t down = mid + (exps / 2) * bracket;

    debug(STR("up: "));
    debug(up);
    debug_nl();
    debug(STR("down: "));
    debug(down);
    debug_nl();
    debug(STR("max: "));
    debug(camera.shutterMax());
    debug_nl();
    debug(STR("min: "));
    debug(camera.shutterMin());
    debug_nl();

    if(up < camera.shutterMax() || down > camera.shutterMin()) return 1; else return 0;
}

uint8_t hdrTvUp(uint8_t ev)
{
    if(checkHDR(timer.current.Exps, ev, timer.current.Bracket))
    {
        uint8_t mid = (camera.shutterMin() - camera.shutterMax()) / 2 + camera.shutterMax();
        if(mid > ev)
        {
            for(uint8_t i = ev; i <= mid; i++)
            {
                if(!checkHDR(timer.current.Exps, i, timer.current.Bracket))
                {
                    ev = i;
                    break;
                }
            }
        }
        else
        {
            for(uint8_t i = ev; i >= mid; i--)
            {
                if(!checkHDR(timer.current.Exps, i, timer.current.Bracket))
                {
                    ev = i;
                    break;
                }
            }
        }
    }
    else
    {
        uint8_t tmp = camera.shutterUp(ev);
        if(!checkHDR(timer.current.Exps, tmp, timer.current.Bracket)) ev = tmp;
    }
    return ev;
}

uint8_t hdrTvDown(uint8_t ev)
{
    if(checkHDR(timer.current.Exps, ev, timer.current.Bracket))
    {
        uint8_t mid = (camera.shutterMin() - camera.shutterMax()) / 2 + camera.shutterMax();
        if(mid > ev)
        {
            for(uint8_t i = ev; i <= mid; i++)
            {
                if(!checkHDR(timer.current.Exps, i, timer.current.Bracket))
                {
                    ev = i;
                    break;
                }
            }
        }
        else
        {
            for(uint8_t i = ev; i >= mid; i--)
            {
                if(!checkHDR(timer.current.Exps, i, timer.current.Bracket))
                {
                    ev = i;
                    break;
                }
            }
        }
    }
    else
    {
        uint8_t tmp = camera.shutterDown(ev);
        if(!checkHDR(timer.current.Exps, tmp, timer.current.Bracket)) ev = tmp;
    }
    return ev;
}

uint8_t bracketUp(uint8_t ev)
{
    uint8_t max = 1;
    for(uint8_t i = 1; i < 255; i++)
    {
        if(checkHDR(timer.current.Exps, timer.current.Exp, i))
        {
            max = i - 1;
            break;
        }
    }
    if(ev < max) ev++; else ev = max;
    return ev;
}

uint8_t bracketDown(uint8_t ev)
{
    if(ev > 1) ev--; else ev = 1;
    return ev;
}

uint8_t hdrExpsUp(uint8_t hdr_exps)
{
    uint8_t max = 1;
    for(uint8_t i = 1; i < 255; i++)
    {
        if(checkHDR(i, timer.current.Exp, timer.current.Bracket))
        {
            max = i - 1;
            if(max < 3) max = 3;
            break;
        }
    }
    if(hdr_exps < max) hdr_exps++; else hdr_exps = max;
    return hdr_exps;
}

uint8_t hdrExpsDown(uint8_t hdr_exps)
{
    if(hdr_exps > 3) hdr_exps--; else hdr_exps = 3;
    return hdr_exps;
}

uint8_t hdrExpsName(char name[8], uint8_t hdr_exps)
{
    name[0] = ' ';
    name[1] = ' ';
    name[2] = ' ';
    name[3] = ' ';
    name[4] = ' ';
    name[5] = ' ';
    name[6] = ' ';
    name[7] = '\0';

    if(hdr_exps > 9)
    {
        name[5] = '0' + (hdr_exps / 10);
        hdr_exps %= 10;
    }
    name[6] = '0' + hdr_exps;

    return 1;
}


