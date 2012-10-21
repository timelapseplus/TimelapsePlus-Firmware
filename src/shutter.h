/*
 *  shutter.h
 *  AVR_Shutter
 *
 *  Created by Elijah Parker on 10/25/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
 
#ifndef shutter_h
#define shutter_h

#define MAX_STORED 25

#define DONE 0
#define CONTINUE 1

#define TIMELAPSE 0x0001
#define HDR 0x0002
#define RAMP 0x0004

#define MODE_TIMELAPSE (TIMELAPSE)
#define MODE_HDR_TIMELAPSE (HDR | TIMELAPSE)
#define MODE_HDR_PHOTO (HDR)
#define MODE_BULB_RAMP (RAMP | TIMELAPSE)
#define MODE_HDR_RAMP (RAMP | HDR | TIMELAPSE)

#define MAX_KEYFRAMES 5

#define DISABLE_SHUTTER setIn(SHUTTER_FULL_PIN);  _delay_ms(50); setLow(SHUTTER_FULL_PIN)
#define DISABLE_MIRROR setIn(SHUTTER_HALF_PIN); _delay_ms(50); setLow(SHUTTER_HALF_PIN)
#define ENABLE_SHUTTER setHigh(SHUTTER_FULL_PIN); setOut(SHUTTER_FULL_PIN); setIn(CHECK_CABLE_PIN); setHigh(CHECK_CABLE_PIN); setIn(SHUTTER_SENSE_PIN); setHigh(SHUTTER_SENSE_PIN);
#define ENABLE_MIRROR setHigh(SHUTTER_HALF_PIN); setOut(SHUTTER_HALF_PIN)

#define ENABLE_AUX_INPUT setOut(AUX_OUT1_PIN); setHigh(AUX_OUT1_PIN); setIn(AUX_INPUT1_PIN); setHigh(AUX_INPUT1_PIN)
#define AUX_INPUT (getPin(AUX_INPUT1_PIN) == 0)


#define MIRROR_UP setLow(SHUTTER_HALF_PIN)
#define MIRROR_DOWN setHigh(SHUTTER_HALF_PIN)
#define SHUTTER_OPEN setLow(SHUTTER_FULL_PIN)
#define SHUTTER_CLOSE setHigh(SHUTTER_FULL_PIN); MIRROR_DOWN

#define MIRROR_IS_DOWN (isHigh(SHUTTER_HALF_PIN))

//#define CHECK_CABLE if(getPin(CHECK_CABLE_PIN) || getPin(SHUTTER_SENSE_PIN)) cable_connected = 1; else cable_connected = 0
#define CHECK_CABLE if(getPin(CHECK_CABLE_PIN)) cable_connected = 1; else cable_connected = 0

struct program
{
    char Name[12];            // 12 bytes
    unsigned int Delay;       // 2 bytes
    unsigned int Photos;      // 2 bytes
    unsigned int Gap;         // 2 bytes
    unsigned int Exps;        // 2 bytes
    unsigned int Mode;        // 2 bytes
    unsigned int Exp;         // 2 bytes
    unsigned int Bracket;     // 2 bytes
    unsigned int Bulb[10];    // 20 bytes
    unsigned int Key[10];     // 20 bytes
    unsigned int Keyframes;   // 2 bytes
};

struct timer_status
{
    char textStatus[12];
    unsigned int photosTaken;
    unsigned int photosRemaining;
    unsigned int nextPhoto;
};

extern program stored[MAX_STORED]EEMEM;

class shutter
{
public:
    shutter();
    void begin(void);
    char run(void);
    void save(char id);
    void load(char id);
    void setDefault(void);
    int8_t nextId(void);

    void off(void);
    void half(void);
    void full(void);

    void bulbEnd(void);
    void bulbStart(void);
    void capture(void);

    char cableIsConnected(void);

    program current;
    timer_status status; 
    volatile char running;  // 0 = not running, 1 = running (shutter closed), 2 = running (shutter open)
    volatile int8_t currentId;
    volatile uint16_t length; // in Seconds

private:
    volatile char cable_connected; // 1 = cable connected, 0 = disconnected
    double test;
};
#endif

