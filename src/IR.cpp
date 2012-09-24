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


IR::IR()
{
  setOut(IR_PIN);
  setHigh(IR_PIN);
}

void IR::high(unsigned int time, int freq){
  uint16_t count = 0;
  cli();
  while(count <= (time / 1000) / (1 / freq))
  {
    setLow(IR_PIN);
    _delay_us((1000/freq/2));
    setHigh(IR_PIN);
    _delay_us((1000/freq/2));
  }
  sei();
}

void IR::shutterNow()
{
  if(make == CANON || make == ALL)
  {
    cli();
    for(int i=0; i<16; i++) { 
      setLow(IR_PIN);
      _delay_us(15.24);
      setHigh(IR_PIN);
      _delay_us(15.24);
    } 
    _delay_ms(7.33); 
    for(int i=0; i<16; i++) { 
      setLow(IR_PIN);
      _delay_us(15.24);
      setHigh(IR_PIN);
      _delay_us(15.24);
    }
    sei();
  }

  if(make == NIKON || make == ALL)
  {
    high(2000,40);
    _delay_ms(27.830);
    high(390,40);
    _delay_ms(1.580);
    high(410,40);
    _delay_ms(3.580);
    high(400,40);
  }

  if(make == PENTAX || make == ALL)
  {
    high(13000,38);
    _delay_ms(3);
    for (int i=0;i<7;i++){
      high(1000,38);
      _delay_ms(1);
    };
  }

  if(make == OLYMPUS || make == ALL)
  {
    bool _seq[] = {
      0,1,1,0,0,0,0,1,1,1,0,1,1,1,0,0,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1    };
    high(8972,40);
    _delay_ms(4.384);
    high(624,40);
    for (int i=0;i<sizeof(_seq);i++){
      if (_seq[i]==0){
        _delay_us(488);
        high(600,40);
      }
      else{
        _delay_ms(1.6);
        high(600,40);
      }
    };
  }

  if(make == MINOLTA || make == ALL)
  {
    bool _seq[] = {
      0,0,1,0,1,1,0,0,0,1,0,1,0,0,1,1,1,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,1    };
    high(3750,38);
    _delay_ms(1.890);
    for (int i=0;i<sizeof(_seq);i++){
      if (_seq[i]==0){
        high(456,38);
        _delay_us(487);
      }
      else{
        high(456,38);
        _delay_ms(1.430);
      }
    };
  }

  if(make == SONY || make == ALL)
  {
    bool _seq[] = {
      1,0,1,1,0,1,0,0,1,0,1,1,1,0,0,0,1,1,1,1    };
    for (int j=0;j<3;j++) {
      high(2320,40);
      _delay_us(650);
      for (int i=0;i<sizeof(_seq);i++){
        if (_seq[i]==0){
          high(575,40);
          _delay_us(650);
        }
        else{
          high(1175,40);
          _delay_us(650);
        }
      }
      _delay_ms(10);
    }
  }

}

void IR::shutterDelayed()
{
  if(make == CANON || make == ALL)
  {
    cli();
    for(int i=0; i<16; i++) { 
      setLow(IR_PIN);
      _delay_us(15.24);
      setHigh(IR_PIN);
      _delay_us(15.24);
    } 
    _delay_ms(5.36); 
    for(int i=0; i<16; i++) { 
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
      0,0,1,0,1,1,0,0,0,1,0,1,0,0,1,1,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1    };
    high(3750,38);
    _delay_ms(1.890);
    for (int i=0;i<sizeof(_seqDelayed);i++){
      if (_seqDelayed[i]==0){
        high(456,38);
        _delay_us(487);
      }
      else{
        high(456,38);
        _delay_ms(1.430);
      }
    };
  }

  if(make == SONY || make == ALL)
  {
    bool _seqDelayed[] = {
      1,1,1,0,1,1,0,0,1,0,1,1,1,0,0,0,1,1,1,1    };
    for (int j=0;j<3;j++) {
      high(2320,40);
      _delay_us(650);
      for (int i=0;i<sizeof(_seqDelayed);i++){
        if (_seqDelayed[i]==0){
          high(575,40);
        }
        else{
          high(1175,40);
        }
        _delay_us(650);
      }
      _delay_ms(10);
    }
  }
}

void IR::zoomIn(unsigned int pct)
{
  if(make == OLYMPUS || make == ALL)
  {
    bool _seq[] = {
      0,1,1,0,0,0,0,1,1,1,0,1,1,1,0,0,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1    };
    high(9000,40);
    _delay_ms(4.500);
    high(500,40);
    for (int i=0;i<sizeof(_seq);i++){
      if (_seq[i]==0){
        _delay_us(500);
      }
      else{
        _delay_ms(1.500);
      }
      high(500,40);
    };
    _delay_ms(40);
    if (pct>100) pct = 100;
    pct = (pct*52)/100 + 1;
    for (int i=1; i<pct; i++)
    {
      high(9000,40);
      _delay_ms(2);
      high(500,40);
      _delay_ms(96);
    }
  }
}

void IR::zoomOut(unsigned int pct)
{
  if(make == OLYMPUS || make == ALL)
  {
    bool _seq[] = 
               {0,1,1,0,0,0,0,1,1,1,0,1,1,1,0,0,0,1,0,0,0,0,0,0,1,0,1,1,1,1,1,1 };
    high(9000,40);
    _delay_ms(4.500);
    high(500,40);
    for (int i=0;i<sizeof(_seq);i++){
      if (_seq[i]==0){
        _delay_us(500);
      }
      else{
        _delay_ms(1.500);
      }
      high(500,40);
    };
    _delay_ms(40);
    if (pct>100) pct = 100;
    pct = (pct*70)/100 + 1;
    for (int i=1; i<pct; i++)
    {
      high(9000,40);
      _delay_ms(2);
      high(500,40);
      _delay_ms(96);
    }
  }
}




