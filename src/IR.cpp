/*******************************************
 *
 * Name.......:  cameraIrControl Library
 * Description:  A powerful Library to control easy various cameras via IR. Please check the project page and leave a comment.
 * Author.....:  Sebastian Setz
 * Version....:  1.7
 * Date.......:  2011-12-07
 * Project....:  http://sebastian.setz.name/arduino/my-libraries/multiCameraIrControl
 * Contact....:  http://Sebastian.Setz.name
 * License....:  This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
 *               To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
 *               Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
 * Keywords...:  arduino, library, camera, ir, control, canon, nikon, olympus, minolta, sony, pentax, interval, timelapse
 * History....:  2010-12-08 V1.0 - release
 *               2010-12-09 V1.1 
 *               2010-12-16 V1.2
 *               2011-01-01 V1.3
 *               2011-01-04 V1.4 - making Sony function work, thank you Walter.
 *               2011-01-25 V1.5 - making Olympus work, thank you Steve Stav.
 *               2011-12-05 V1.6 - adding Olympus zoom, thank you again Steve! Refresh keywords.txt; Arduino 1.0 compatible
 *               2011-12-07 V1.7 - repairing Canon function - thanks to Viktor
 *
 ********************************************/

#include "IR.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "hardware.h"
#include "settings.h"
#include "clock.h"

extern Clock clock;

IR::IR()
{
    setOut(IR_PIN);
    setHigh(IR_PIN);
}

void IR::high40(unsigned int time)
{
    uint16_t count = 0;
        
    while (count <= time / (1000 / 40))
    {
        setLow(IR_PIN);
        _delay_us((1000 / 40 / 2));
        setHigh(IR_PIN);
        _delay_us((1000 / 40 / 2));
        count++;
    }
}

void IR::high38(unsigned int time)
{
    uint16_t count = 0;
    
    while (count <= time / (1000 / 38))
    {
        setLow(IR_PIN);
        _delay_us((1000 / 38 / 2));
        setHigh(IR_PIN);
        _delay_us((1000 / 38 / 2));
        count++;
    }
}

void IR::shutterNow()
{
    if(make == CANON || make == ALL)
    {        
        cli();
        for(int i = 0; i < 16; i++)
        {
            setLow(IR_PIN);
            _delay_us(15.24);
            setHigh(IR_PIN);
            _delay_us(15.24);
        }
        
        _delay_ms(7.33);
        
        for(int i = 0; i < 16; i++)
        {
            setLow(IR_PIN);
            _delay_us(15.24);
            setHigh(IR_PIN);
            _delay_us(15.24);
        }
        clock.advance((uint8_t) (7.33 + 0.01524 * 4 * 16));
        sei();
    }

    if(make == NIKON || make == ALL)
    {
        cli();
        high40(2000);
        _delay_ms(27.830);
        high40(390);
        _delay_ms(1.580);
        high40(410);
        _delay_ms(3.580);
        high40(400);

        clock.advance((uint8_t) (2 + 0.39 + 0.41 + 0.4 + 27.83 + 1.58 + 3.58));
        sei();
    }

    if(make == PENTAX || make == ALL)
    {
        cli();
        high38(13000);
        _delay_ms(3);
        
        for(int i = 0; i < 7; i++)
        {
            high38(1000);
            _delay_ms(1);
        };
        clock.advance((uint8_t) (14 + 3 + 13));
        sei();
    }

    if(make == OLYMPUS || make == ALL)
    {
        bool _seq[] = {
            0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1 };
        
        cli();
        high40(8972);
        _delay_ms(4.384);
        high40(624);
        
        for(uint8_t i = 0; i < sizeof(_seq)/sizeof(_seq[0]); i++)
        {
            if(_seq[i] == 0)
            {
                _delay_us(488);
                high40(600);
            } else
            {
                _delay_ms(1.6);
                high40(600);
            }
        };
        clock.advance((uint8_t) (89.72 + 4.384 + 0.624 + ((0.488 + 0.6) * 16) + ((1.6 + 0.6) * 16)));
        sei();
    }

    if(make == MINOLTA || make == ALL)
    {
        bool _seq[] = {
            0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1 };
        
        cli();

        high38(3750);
        _delay_ms(1.890);
        
        for(uint8_t i = 0; i < sizeof(_seq)/sizeof(_seq[0]); i++)
        {
            if(_seq[i] == 0)
            {
                high38(456);
                _delay_us(487);
            } else
            {
                high38(456);
                _delay_ms(1.430);
            }
        };
        clock.advance((uint8_t) (3.75 + 1.89 + ((0.456 + 0.487) * 16) + ((0.456 + 1.43) * 16)));
        sei();
    }

    if(make == SONY || make == ALL)
    {
        bool _seq[] = {
            1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1 };

        cli();

        for(int j = 0; j < 3; j++)
        {
            high40(2320);
            _delay_us(650);
            
            for(uint8_t i = 0; i < sizeof(_seq)/sizeof(_seq[0]); i++)
            {
                if(_seq[i] == 0)
                {
                    high40(575);
                    _delay_us(650);
                } else
                {
                    high40(1175);
                    _delay_us(650);
                }
            }
            _delay_ms(10);
        }
        clock.advance((uint8_t) (2.32 + 0.65 + (0.575 + 650) * 8 + (1.175 + 650) * 12));
        sei();
    }

}

void IR::shutterDelayed()
{
    if(make == CANON || make == ALL)
    {
        cli();
        
        for(int i = 0; i < 16; i++)
        {
            setLow(IR_PIN);
            _delay_us(15.24);
            setHigh(IR_PIN);
            _delay_us(15.24);
        }
        
        _delay_ms(5.36);
        
        for(int i = 0; i < 16; i++)
        {
            setLow(IR_PIN);
            _delay_us(15.24);
            setHigh(IR_PIN);
            _delay_us(15.24);
        }
        
        sei();
    }

    if(make == MINOLTA || make == ALL)
    {
        bool _seqDelayed[] = {
            0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
        
        cli();

        high38(3750);
        _delay_ms(1.890);
        
        for(uint8_t i = 0; i < sizeof(_seqDelayed)/sizeof(_seqDelayed[0]); i++)
        {
            if(_seqDelayed[i] == 0)
            {
                high38(456);
                _delay_us(487);
            } else
            {
                high38(456);
                _delay_ms(1.430);
            }
        };
        sei();
    }

    if(make == SONY || make == ALL)
    {
        bool _seqDelayed[] = {
            1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1 };
        
        cli();

        for(uint8_t j = 0; j < 3; j++)
        {
            high40(2320);
            _delay_us(650);
            
            for(uint8_t i = 0; i < sizeof(_seqDelayed)/sizeof(_seqDelayed[0]); i++)
            {
                if(_seqDelayed[i] == 0)
                {
                    high40(575);
                } else
                {
                    high40(1175);
                }
                _delay_us(650);
            }
            _delay_ms(10);
        }
        sei();
    }
}

void IR::zoomIn(unsigned int pct)
{
    if(make == OLYMPUS || make == ALL)
    {
        bool _seq[] = {
            0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 };
        
        cli();

        high40(9000);
        _delay_ms(4.500);
        high40(500);
        
        for(uint8_t i = 0; i < sizeof(_seq)/sizeof(_seq[0]); i++)
        {
            if(_seq[i] == 0)
            {
                _delay_us(500);
            } else
            {
                _delay_ms(1.500);
            }
            high40(500);
        };
        
        _delay_ms(40);
        if(pct > 100) pct = 100;
        pct = (pct * 52) / 100 + 1;
        
        for(uint8_t i = 1; i < pct; i++)
        {
            high40(9000);
            _delay_ms(2);
            high40(500);
            _delay_ms(96);
        }
        sei();
    }
}

void IR::zoomOut(unsigned int pct)
{
    if(make == OLYMPUS || make == ALL)
    {
        bool _seq[] =
        { 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1 };
        
        cli();
        high40(9000);
        _delay_ms(4.500);
        high40(500);
        
        for(uint8_t i = 0; i < sizeof(_seq)/sizeof(_seq[0]); i++)
        {
            if(_seq[i] == 0)
            {
                _delay_us(500);
            } else
            {
                _delay_ms(1.500);
            }
            high40(500);
        };
        
        _delay_ms(40);
        
        if(pct > 100) 
            pct = 100;
        
        pct = (pct * 70) / 100 + 1;
        
        for(uint8_t i = 1; i < pct; i++)
        {
            high40(9000);
            _delay_ms(2);
            high40(500);
            _delay_ms(96);
        }

        sei();
    }
}

void IR::bulbStart()
{
    if(make == OLYMPUS)
    {
        zoomOut(10);
    }
    else
    {
        shutterNow();
    }
}


void IR::bulbEnd()
{
    if(make == OLYMPUS)
    {
        zoomIn(10);
    }
    else
    {
        shutterNow();
    }
}

