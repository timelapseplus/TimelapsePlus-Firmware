#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include "PTP_Driver.h"
#include "PTP.h"
#include "PTP_Codes.h"
#include "shutter.h"
#include "settings.h"
#include "tldefs.h"
#include "debug.h"

//#define EXTENDED_DEBUG

extern settings conf;

const propertyDescription_t PTP_Aperture_List[] PROGMEM = {
    {"  Error", 0xFF, 0xFF, 255 },
    {"  f/1.2", 0x0d, 120, 2 },
    {"  f/1.4", 0x10, 140, 3 },
    {"  f/1.6", 0x13, 160, 4 },
    {"  f/1.8", 0x15, 180, 5 },
    {"  f/2.0", 0x18, 200, 6 },
    {"  f/2.2", 0x1b, 220, 7 },
    {"  f/2.5", 0x1d, 250, 8 },
    {"  f/2.8", 0x20, 280, 9 },
    {"  f/3.2", 0x23, 320, 10 },
    {"  f/3.5", 0x25, 350, 11 },
    {"  f/4.0", 0x28, 400, 12 },
    {"  f/4.5", 0x2b, 450, 13 },
    {"  f/5.0", 0x2d, 500, 14 },
    {"  f/5.6", 0x30, 560, 15 },
    {"  f/6.3", 0x33, 630, 16 },
    {"  f/7.1", 0x35, 710, 17 },
    {"  f/8.0", 0x38, 800, 18 },
    {"  f/9.0", 0x3b, 900, 19 },
    {"   f/10", 0x3d, 1000, 20 },
    {"   f/11", 0x40, 1100, 21 },
    {"   f/13", 0x43, 1300, 22 },
    {"   f/14", 0x45, 1400, 23 },
    {"   f/16", 0x48, 1600, 24 },
    {"   f/18", 0x4b, 1800, 25 },
    {"   f/20", 0x4d, 2000, 26 },
    {"   f/22", 0x50, 2200, 27 },
    {"   f/25", 0x53, 2500, 28 },
    {"   f/29", 0x55, 2900, 29 },
    {"   f/32", 0x58, 3200, 30 },
    {"   f/36", 0x5b, 3600, 31 }
};

const propertyDescription_t PTP_Shutter_List[] PROGMEM = {
    {"  Error", 0xFF, 0x000000FF, 255 },
    {" 1/8000", 0xa0, 0x00000000, 82 },
    {" 1/6400", 0x9d, 0x00000000, 81 },
    {" 1/5000", 0x9b, 0x00000000, 80 },
    {" 1/4000", 0x98, 2,      79 },
    {" 1/3200", 0x95, 3,      78 },
    {" 1/2500", 0x93, 4,      77 },
    {" 1/2000", 0x90, 5,      76 },
    {" 1/1600", 0x8d, 6,      75 },
    {" 1/1250", 0x8b, 8,      74 },
    {" 1/1000", 0x88, 10,     73 },
    {"  1/800", 0x85, 12,     72 },
    {"  1/640", 0x83, 15,     71 },
    {"  1/500", 0x80, 20,     70 },
    {"  1/400", 0x7d, 25,     69 },
    {"  1/320", 0x7b, 31,     68 },
    {"  1/250", 0x78, 40,     67 },
    {"  1/200", 0x75, 50,     66 },
    {"  1/160", 0x73, 62,     65 },
    {"  1/125", 0x70, 80,     64 },
    {"  1/100", 0x6d, 100,    63 },
    {"   1/80", 0x6b, 125,    62 },
    {"   1/60", 0x68, 166,    61 },
    {"   1/50", 0x65, 200,    60 },
    {"   1/40", 0x63, 250,    59 },
    {"   1/30", 0x60, 333,    58 },
    {"   1/25", 0x5d, 400,    57 },
    {"   1/20", 0x5b, 500,    56 },
    {"   1/15", 0x58, 666,    55 },
    {"   1/13", 0x55, 769,    54 },
    {"   1/10", 0x53, 1000,   53 },
    {"    1/8", 0x50, 1250,   52 },
    {"    1/6", 0x4d, 1666,   51 },
    {"    1/5", 0x4b, 2000,   50 },
    {"    1/4", 0x48, 2500,   49 },
    {"    0.3", 0x45, 3333,   48 },
    {"    0.4", 0x43, 4000,   47 },
    {"    0.5", 0x40, 5000,   46 },
    {"    0.6", 0x3d, 6250,   45 },
    {"    0.8", 0x3b, 7692,   44 },
    {"     1s", 0x38, 10000,  43 },
    {"   1.3s", 0x35, 13000,  42 },
    {"   1.6s", 0x33, 16000,  41 },
    {"     2s", 0x30, 20000,  40 },
    {"   2.5s", 0x2d, 25000,  39 },
    {"   3.2s", 0x2b, 30000,  38 },
    {"     4s", 0x28, 40000,  37 },
    {"     5s", 0x25, 50000,  36 },
    {"     6s", 0x23, 60000,  35 },
    {"     8s", 0x20, 80000,  34 },
    {"    10s", 0x1d, 100000, 33 },
    {"    13s", 0x1b, 130000, 32 },
    {"    15s", 0x18, 150000, 31 },
    {"    20s", 0x15, 200000, 30 },
    {"    25s", 0x13, 250000, 29 },
    {"    30s", 0x10, 300000, 28 },
    {"   Bulb", 0x0c, 0xffffffff, 253 }
};

const propertyDescription_t PTP_ISO_List[] PROGMEM = {
    {"  Error", 0xFF, 0x000000FF, 255 },
    {"   Auto", 0x00, 0x00000000, 254 },
    {"     50", 0x40, 50, 46 },
    {"    100", 0x48, 100, 43 },
    {"    125", 0x4b, 125, 42 },
    {"    160", 0x4d, 160, 41 },
    {"    200", 0x50, 200, 40 },
    {"    250", 0x53, 250, 39 },
    {"    320", 0x55, 320, 38 },
    {"    400", 0x58, 400, 37 },
    {"    500", 0x5b, 500, 36 },
    {"    640", 0x5d, 640, 35 },
    {"    800", 0x60, 800, 34 },
    {"   1000", 0x63, 1000, 33 },
    {"   1250", 0x65, 1250, 32 },
    {"   1600", 0x68, 1600, 31 },
    {"   2000", 0x6B, 2000, 30 },
    {"   2500", 0x6D, 2500, 29 },
    {"   3200", 0x70, 3200, 28 },
    {"   4000", 0x73, 4000, 27 },
    {"   5000", 0x75, 5000, 26 },
    {"   6400", 0x78, 6400, 25 },
    {"   8000", 0x7B, 8000, 24 },
    {"  10000", 0x7D, 10000, 23 },
    {"  12800", 0x80, 12800, 22 },
    {"  16000", 0x83, 16000, 21 },
    {"  20000", 0x85, 20000, 20 },
    {"  25600", 0x88, 25600, 19 },
    {"  32000", 0x8B, 32000, 18 },
    {"  40000", 0x8D, 40000, 17 },
    {"  51200", 0x90, 51200, 16 },
    {"  64000", 0x93, 64000, 15 },
    {"  81000", 0x95, 81000, 14 },
    {" 102400", 0x98, 102400, 13 },
    {" 129000", 0x9B, 129000, 12 },
    {" 162000", 0x9D, 162000, 11 },
    {" 204800", 0xA0, 204800, 10 }
}; 

// Property lists:
// ID   Type* ?  ?  1st      Current  ?   ArrayLen list...
// 0F50 04    00 01 C800     9001     02  0500     C800 9001 2003 4006 800C 
// 0F50 04    00 01 C800     800C     02  0500     C800 9001 2003 4006 800C
// 0D50 06    00 01 02000000 60EA0000 02  3500     02000000 03000000 04 00 00 00 05 00 00 00 06 00 00 00 08 00 00 00 0A 00 00 00 0C 00 00 00 0F 00 00 00 14 00 00 00 19 00 00 00 1F 00 00 00 28 00 00 00 32 00 00 00 3E 00 00 00 50 00 00 00 64 00 00 00 7D 00 00 00 A6 00 00 00 C8 00 00 00 FA 00 00 00 4D 01 00 00 90 01 00 00 F4 01 00 00 9A 02 00 00 01 03 00 00 E8 03 00 00 E2 04 00 00 82 06 00 00 D0 07 00 00 C4 09 00 00 05 0D 00 00 A0 0F 00 00 88 13 00 00 6A 18 00 00 0C 1E 00 00 10 27 00 00 C8 32 00 00 80 3E 00 00 20 4E 00 00 A8 61 00 00 30 75 00 00 40 9C 00 00 50 C3 00 00 60 EA 00 00 80 38 01 00 A0 86 01 00 D0 FB 01 00 F0 49 02 00 40 0D 03 00 90 D0 03 00 E0 93 04 00 FF FF FF FF
// 0D50 06    00 01 02000000 02000000 02  3500     02000000 03000000 04 00 00 00 05 00 00 00 06 00 00 00 08 00 00 00 0A 00 00 00 0C 00 00 00 0F 00 00 00 14 00 00 00 19 00 00 00 1F 00 00 00 28 00 00 00 32 00 00 00 3E 00 00 00 50 
// 0750 04    00 01 A401     3002     02  1200     A401 C201 F401 3002 76 02 C6 02 20 03 84 03 E8 03 4C 04 14 05 78 05 40 06 08 07 D0 07 98 08 C4 09 54 0B 

//*Types:
// 1 = INT8
// 2 = UINT8
// 3 = INT16
// 4 = UINT16
// 5 = INT32
// 6 = UINT32
// 7 = INT64
// 8 = UINT64

//ISO:
// 200 = C8 00
// 400 = 90 01
// 800 = 20 03
//1600 = 40 06
//3200 = 80 0C


const bulbSettings_t Bulb_List[] PROGMEM = {
    {" Camera", 0,  254 },
    {"   1/20", 49, 56 },
    {"   1/15", 62, 55 },
    {"   1/13", 79, 54 },
    {"   1/10", 99, 53 },
    {"    1/8", 125, 52 },
    {"    1/6", 157, 51 },
    {"    1/5", 198, 50 },
    {"    1/4", 250, 49 },
    {"   0.3s", 315, 48 },
    {"   0.4s", 397, 47 },
    {"   0.5s", 500, 46 },
    {"   0.6s", 630, 45 },
    {"   0.8s", 794, 44 },
    {"     1s", 1000, 43 },
    {"   1.3s", 1260, 42 },
    {"   1.6s", 1587, 41 },
    {"     2s", 2000, 40 },
    {"   2.5s", 2520, 39 },
    {"   3.2s", 3175, 38 },
    {"     4s", 4000, 37 },
    {"     5s", 5040, 36 },
    {"     6s", 6349, 35 },
    {"     8s", 8000, 34 },
    {"    10s", 10079, 33 },
    {"    13s", 12699, 32 },
    {"    15s", 16000, 31 },
    {"    20s", 20158, 30 },
    {"    25s", 25398, 29 },
    {"    30s", 32000, 28 },
    {"    40s", 40320, 27 },
    {"    50s", 50800, 26 },
    {"    60s", 64000, 25 },
    {"    80s", 80630, 24 },
    {"   100s", 101590, 23 },
    {"   125s", 128000, 22 },
    {"   160s", 161270, 21 },
    {"   200s", 203180, 20 },
    {"     4m", 256000, 19 },
    {"   5.3m", 322530, 18 },
    {"   6.6m", 406360, 17 },
    {"     8m", 512000, 16 },
    {"    11m", 645070, 15 },
    {"    13m", 812720, 14 },
    {"    15m", 1024000, 13 },
    {"    20m", 1290138, 12 },
    {"    25m", 1625444, 11 },
    {"    30m", 2048000, 10 },
    {"    40m", 2580318, 9 },
    {"    50m", 3250997, 8 },
    {"    60m", 4096000, 7 },
    {"    80m", 5160637, 6 },
    {"   100m", 6501995, 5 },
    {"   125m", 8192000, 4 }
};


uint8_t isoAvail[32];
uint8_t isoAvailCount;
uint8_t shutterAvail[64];
uint8_t shutterAvailCount;
uint8_t apertureAvail[32];
uint8_t apertureAvailCount;

uint8_t PTP_need_update = 1;

uint32_t isoPTP;
uint32_t shutterPTP;
uint32_t aperturePTP;
uint32_t modePTP;

uint8_t preBulbShutter;

uint8_t static_ready;

uint8_t PTP_protocol;
uint16_t PTP_propertyOffset;

uint32_t currentObject;


PTP::PTP(void)
{
	static_ready = 0;
	ready = 0;
	isoAvailCount = 0;
	apertureAvailCount = 0;
	shutterAvailCount = 0;
	PTP_protocol = 0;
	currentObject = 0;
}

uint8_t PTP::bulbMax() // 7
{
	return pgm_read_byte(&Bulb_List[sizeof(Bulb_List) / sizeof(Bulb_List[0]) - 1].ev);
}

uint8_t PTP::bulbMin() // 56 (46 for IR)
{
	uint8_t tmp = pgm_read_byte(&Bulb_List[1].ev);
	if(tmp > conf.bulbMin) tmp = conf.bulbMin;
	return tmp;
}

uint8_t PTP::isoMax() // 13
{
	uint8_t tmp = 0;
	if(isoAvailCount > 0)
	{
		for(uint8_t i = 1; i <= isoAvailCount; i++)
		{
			tmp = isoAvail[isoAvailCount - i];
			if(tmp > 0 && tmp < 128) break;
		}
		if(tmp < conf.isoMax) tmp = conf.isoMax;
	}
	return tmp;
}

uint8_t PTP::isoMin() // 46
{
	if(isoAvailCount > 0)
	{
		for(uint8_t i = 0; i < isoAvailCount; i++)
		{
			uint8_t tmp = isoAvail[i];
			if(tmp > 0 && tmp < 128) return tmp;
		}
	}
	return 0;
}

uint8_t PTP::apertureMax() // 13
{
	uint8_t tmp = 0;
	if(apertureAvailCount > 0)
	{
		for(uint8_t i = 1; i <= apertureAvailCount; i++)
		{
			uint8_t tmp = apertureAvail[apertureAvailCount - i];
			if(tmp > 0 && tmp < 128) break;
		}
		if(tmp > conf.apertureMax) tmp = conf.apertureMax;
	}
	return tmp;
}

uint8_t PTP::apertureMin() // 2
{
	if(apertureAvailCount > 0)
	{
		for(uint8_t i = 0; i < apertureAvailCount; i++)
		{
			uint8_t tmp = apertureAvail[i];
			if(tmp > 0 && tmp < 128) return tmp;
		}
	}
	return 0;
}

uint8_t PTP::shutterMax() // 7
{
	return bulbMax();
}

uint8_t PTP::shutterMin() // 76
{
	if(shutterAvailCount > 0)
	{
		for(uint8_t i = shutterAvailCount; i > 0; i--)
		{
			uint8_t tmp = shutterAvail[i];
			if(tmp > 0 && tmp < 128) return tmp;
		}
	}
	return bulbMin();
}

uint8_t PTP::isoUpStatic(uint8_t ev)
{
	for(uint8_t i = 0; i < sizeof(PTP_ISO_List) / sizeof(PTP_ISO_List[0]); i++)
	{
		if(pgm_read_byte(&PTP_ISO_List[i].ev) == ev)
		{
			if(i - 1 > 1)
			return pgm_read_byte(&PTP_ISO_List[i - 1].ev);
		}
	}
	return pgm_read_byte(&PTP_ISO_List[2].ev);
}

uint8_t PTP::isoDownStatic(uint8_t ev)
{
	for(uint8_t i = 0; i < sizeof(PTP_ISO_List) / sizeof(PTP_ISO_List[0]); i++)
	{
		if(pgm_read_byte(&PTP_ISO_List[i].ev) == ev)
		{
			if(i + 1 < (uint8_t) (sizeof(PTP_ISO_List) / sizeof(PTP_ISO_List[0])))
			return pgm_read_byte(&PTP_ISO_List[i + 1].ev);
		}
	}
	return pgm_read_byte(&PTP_ISO_List[sizeof(PTP_ISO_List) / sizeof(PTP_ISO_List[0]) - 1].ev);
}

uint8_t PTP::apertureUpStatic(uint8_t ev)
{
	for(uint8_t i = 0; i < sizeof(PTP_Aperture_List) / sizeof(PTP_Aperture_List[0]); i++)
	{
		if(pgm_read_byte(&PTP_Aperture_List[i].ev) == ev)
		{
			if(i - 1 > 1)
			return pgm_read_byte(&PTP_Aperture_List[i - 1].ev);
		}
	}
	return pgm_read_byte(&PTP_Aperture_List[2].ev);
}

uint8_t PTP::apertureDownStatic(uint8_t ev)
{
	for(uint8_t i = 0; i < sizeof(PTP_Aperture_List) / sizeof(PTP_Aperture_List[0]); i++)
	{
		if(pgm_read_byte(&PTP_Aperture_List[i].ev) == ev)
		{
			if(i + 1 < (uint8_t) (sizeof(PTP_Aperture_List) / sizeof(PTP_Aperture_List[0])))
			return pgm_read_byte(&PTP_Aperture_List[i + 1].ev);
		}
	}
	return pgm_read_byte(&PTP_Aperture_List[sizeof(PTP_Aperture_List) / sizeof(PTP_Aperture_List[0]) - 1].ev);
}

uint8_t PTP::isoUp(uint8_t ev)
{
	if(isoAvailCount == 0) return ev;
	for(uint8_t i = 0; i < isoAvailCount; i++)
	{
		if(isoAvail[i] == ev)
		{
			if(i < isoAvailCount - 1)
			{
				return isoAvail[i + 1];
			}
			else
			{
				break;
			}
		}
	}
	return isoAvail[isoAvailCount - 1];
}

uint8_t PTP::isoDown(uint8_t ev)
{
	if(isoAvailCount == 0) return ev;
	for(uint8_t i = 0; i < isoAvailCount; i++)
	{
		if(isoAvail[i] == ev)
		{
			if(i > 0)
			{
				return isoAvail[i - 1];
			}
			else
			{
				break;
			}
		}
	}
	return isoAvail[0];
}

uint8_t PTP::shutterUp(uint8_t ev)
{
	if(ev < 128)
	{
		for(uint8_t i = 0; i < shutterAvailCount; i++)
		{
			if(shutterAvail[i] == ev + 1)
			{
				return shutterAvail[i];
			}
		}
		for(uint8_t i = 0; i < sizeof(Bulb_List) / sizeof(Bulb_List[0]); i++)
		{
			if(pgm_read_byte(&Bulb_List[i].ev) == ev + 1)
			{
				return pgm_read_byte(&Bulb_List[i].ev);
			}
		}	
	}
	if(PTP::shutterType(ev))
	{
		if(shutterAvailCount == 0)
		{
			return pgm_read_byte(&Bulb_List[0].ev);
		}
		return ev;
	}
	else
	{
		if(shutterAvailCount > 0)
		{
			return shutterAvail[shutterAvailCount - 1];
		}
		else
		{
			return pgm_read_byte(&Bulb_List[0].ev);
		}
	}
}

uint8_t PTP::shutterDown(uint8_t ev)
{
	for(uint8_t i = 0; i < shutterAvailCount; i++)
	{
		if(shutterAvail[i] == ev - 1)
		{
			return shutterAvail[i];
		}
	}
	for(uint8_t i = 0; i < sizeof(Bulb_List) / sizeof(Bulb_List[0]); i++)
	{
		if(pgm_read_byte(&Bulb_List[i].ev) == (ev - 1))
		{
			return pgm_read_byte(&Bulb_List[i].ev);
		}
	}
	if(ev > 128) return pgm_read_byte(&Bulb_List[1].ev);
	if(PTP::shutterType(ev))
	{
		return ev;
	}
	else
	{
		if(shutterAvailCount > 0)
		{
			return shutterAvail[0];
		}
		else
		{
			return pgm_read_byte(&Bulb_List[1].ev);
		}
	}
}

uint8_t PTP::bulbUp(uint8_t ev)
{
	for(uint8_t i = 0; i < sizeof(Bulb_List) / sizeof(Bulb_List[0]); i++)
	{
		if(pgm_read_byte(&Bulb_List[i].ev) == ev + 1)
		{
			return pgm_read_byte(&Bulb_List[i].ev);
		}
	}
	if(PTP::shutterType(ev))
	{
		return ev;
	}
	else
	{
		return pgm_read_byte(&Bulb_List[1].ev);
	}
}

uint8_t PTP::bulbDown(uint8_t ev)
{
	for(uint8_t i = 0; i < sizeof(Bulb_List) / sizeof(Bulb_List[0]); i++)
	{
		if(pgm_read_byte(&Bulb_List[i].ev) == (ev - 1))
		{
			return pgm_read_byte(&Bulb_List[i].ev);
		}
	}
	if(PTP::shutterType(ev))
	{
		return ev;
	}
	else
	{
		return pgm_read_byte(&Bulb_List[1].ev);
	}
}

uint8_t PTP::apertureUp(uint8_t ev)
{
	for(uint8_t i = 0; i < apertureAvailCount; i++)
	{
		if(apertureAvail[i] == ev)
		{
			if(i < apertureAvailCount - 1)
			{
				return apertureAvail[i + 1];
			}
			else
			{
				break;
			}
		}
	}
	return apertureAvail[apertureAvailCount - 1];
}

uint8_t PTP::apertureDown(uint8_t ev)
{
	for(uint8_t i = 0; i < apertureAvailCount; i++)
	{
		if(apertureAvail[i] == ev)
		{
			if(i > 0)
			{
				return apertureAvail[i - 1];
			}
			else
			{
				break;
			}
		}
	}
	return apertureAvail[0];
}

uint8_t PTP::iso()
{
	if(isoAvailCount > 0) return PTP::isoEv(isoPTP); else return 0;
}

uint8_t PTP::shutter()
{
	if(shutterAvailCount > 0) return PTP::shutterEv(shutterPTP); else return 0;
}

uint8_t PTP::aperture()
{
	if(apertureAvailCount > 0) return PTP::apertureEv(aperturePTP); else return 0;
}

uint8_t PTP::isoEv(uint32_t id)
{
	for(uint8_t i = 0; i < sizeof(PTP_ISO_List) / sizeof(PTP_ISO_List[0]); i++)
	{
		if(pgm_read_u32((uint8_t*)&PTP_ISO_List[i] + PTP_propertyOffset) == id)
		{
			return pgm_read_byte(&PTP_ISO_List[i].ev);
		}
	}
	return 0;
}

uint8_t PTP::shutterEv(uint32_t id)
{
	for(uint8_t i = 0; i < sizeof(PTP_Shutter_List) / sizeof(PTP_Shutter_List[0]); i++)
	{
		if(pgm_read_u32((uint8_t*)&PTP_Shutter_List[i] + PTP_propertyOffset) == id)
		{
			return pgm_read_byte(&PTP_Shutter_List[i].ev);
		}
	}
	return 0;
}

uint8_t PTP::apertureEv(uint32_t id)
{
	for(uint8_t i = 0; i < sizeof(PTP_Aperture_List) / sizeof(PTP_Aperture_List[0]); i++)
	{
		if(pgm_read_u32((uint8_t*)&PTP_Aperture_List[i] + PTP_propertyOffset) == id)
		{
			return pgm_read_byte(&PTP_Aperture_List[i].ev);
		}
	}
	return 0;
}

uint32_t PTP::isoEvPTP(uint8_t ev)
{
	for(uint8_t i = 0; i < sizeof(PTP_ISO_List) / sizeof(PTP_ISO_List[0]); i++)
	{
		if(pgm_read_byte(&PTP_ISO_List[i].ev) == ev)
		{
			return pgm_read_u32((uint8_t*)&PTP_ISO_List[i] + PTP_propertyOffset);
		}
	}
	return 0;
}

uint32_t PTP::shutterEvPTP(uint8_t ev)
{
	for(uint8_t i = 0; i < sizeof(PTP_Shutter_List) / sizeof(PTP_Shutter_List[0]); i++)
	{
		if(pgm_read_byte(&PTP_Shutter_List[i].ev) == ev)
		{
			return pgm_read_u32((uint8_t*)&PTP_Shutter_List[i] + PTP_propertyOffset);
		}
	}
	return 0;
}

uint32_t PTP::apertureEvPTP(uint8_t ev)
{
	for(uint8_t i = 0; i < sizeof(PTP_Aperture_List) / sizeof(PTP_Aperture_List[0]); i++)
	{
		if(pgm_read_byte(&PTP_Aperture_List[i].ev) == ev)
		{
			return pgm_read_u32((uint8_t*)&PTP_Aperture_List[i] + PTP_propertyOffset);
		}
	}
	return 0;
}

uint8_t PTP::isoName(char name[8], uint8_t ev)
{
	for(uint8_t i = 0; i < sizeof(PTP_ISO_List) / sizeof(PTP_ISO_List[0]); i++)
	{
		if(pgm_read_byte(&PTP_ISO_List[i].ev) == ev)
		{
			if(name)
			{
				for(uint8_t b = 0; b < 8; b++) name[b] = pgm_read_byte(&PTP_ISO_List[i].name[b]);
			}
			return 1;
		}
	}
	return 0;
}

uint8_t PTP::apertureName(char name[8], uint8_t ev)
{
	for(uint8_t i = 0; i < sizeof(PTP_Aperture_List) / sizeof(PTP_Aperture_List[0]); i++)
	{
		if(pgm_read_byte(&PTP_Aperture_List[i].ev) == ev)
		{
			if(name)
			{
				for(uint8_t b = 0; b < 8; b++) name[b] = pgm_read_byte(&PTP_Aperture_List[i].name[b]);
			}
			return 1;
		}
	}
	return 0;
}

uint8_t PTP::shutterName(char name[8], uint8_t ev)
{
	for(uint8_t i = 0; i < sizeof(PTP_Shutter_List) / sizeof(PTP_Shutter_List[0]); i++)
	{
		if(pgm_read_byte(&PTP_Shutter_List[i].ev) == ev)
		{
			if(name)
			{
				for(uint8_t b = 0; b < 8; b++) name[b] = pgm_read_byte(&PTP_Shutter_List[i].name[b]);
			}
			return 1;
		}
	}
	for(uint8_t i = 0; i < sizeof(Bulb_List) / sizeof(Bulb_List[0]); i++)
	{
		if(pgm_read_byte(&Bulb_List[i].ev) == ev)
		{
			if(name)
			{
				for(uint8_t b = 0; b < 8; b++) name[b] = pgm_read_byte(&Bulb_List[i].name[b]);
			}
			return 2;
		}
	}
	return 0;
}

uint8_t PTP::bulbName(char name[8], uint16_t bulb_time)
{
	name[0] = '\0';
	if(bulb_time == 0) return 0;
	for(uint8_t i = 0; i < sizeof(Bulb_List) / sizeof(Bulb_List[0]); i++)
	{
		if(pgm_read_word(&Bulb_List[i].ms) >= bulb_time)
		{
			if(name)
			{
				for(uint8_t b = 0; b < 8; b++) name[b] = pgm_read_byte(&Bulb_List[i].name[b]);
			}
			return 2;
		}
	}
	return 0;
}

uint8_t PTP::shutterType(uint8_t ev)
{
	uint8_t ret = 0;
	for(uint8_t i = 0; i < shutterAvailCount; i++)
	{
		if(shutterAvail[i] == ev)
		{
			ret |= SHUTTER_MODE_PTP;
			break;
		}
	}
	for(uint8_t i = 0; i < sizeof(Bulb_List) / sizeof(Bulb_List[0]); i++)
	{
		if(pgm_read_byte(&Bulb_List[i].ev) == ev)
		{
			if(ev < 128) ret |= SHUTTER_MODE_BULB;
			break;
		}
	}
	return ret;
}

uint32_t PTP::bulbTime(float ev)
{
	if(ev == 0) return 0;
	int8_t d = (int8_t)floor(ev);
	float ms = (float) bulbTime(d);
	ev -= (float)d;
	ev *= 10;
	d = (int8_t)floor(ev);
	for(int8_t i = 0; i < d; i++)
	{
		ms /= THIRTIETH_ROOT_OF_2;
	}
	ev -= (float)d;
	ev *= 10;
	d = (int8_t)floor(ev);
	for(int8_t i = 0; i < d; i++)
	{
		ms /= THREE_HUNDREDTH_ROOT_OF_2;
	}

	return ms;
}


uint32_t PTP::bulbTime(int8_t ev)
{
	if(ev == 0) return 0;
	uint32_t ms = 0;
	for(uint8_t i = 0; i < sizeof(Bulb_List) / sizeof(Bulb_List[0]); i++)
	{
		if(pgm_read_byte(&Bulb_List[i].ev) == ev)
		{
			char *ptr1, *ptr2;
			ptr1 = (char *) &ms;
			ptr2 = (char *) &Bulb_List[i].ms;
			ptr1[0] = pgm_read_byte(&ptr2[0]);
			ptr1[1] = pgm_read_byte(&ptr2[1]);
			ptr1[2] = pgm_read_byte(&ptr2[2]);
			ptr1[3] = pgm_read_byte(&ptr2[3]);
			return ms;
		}
	}
	// Reaching outside of predefined list
	if(ev < bulbMax())
	{
		int8_t diff = bulbMax() - ev;
		return shiftBulb(bulbTime((int8_t)bulbMax()), diff);
	}
	else
	{
		int8_t diff = ev - bulbMin();
		return shiftBulb(bulbTime((int8_t)bulbMin()), diff);
	}
}

uint32_t PTP::shiftBulb(uint32_t ms, int8_t ev)
{
	float f = (float) ms;
	if(ev > 0)
	{
		for(int8_t i = 0; i < ev; i++)
		{
			f *= CUBE_ROOT_OF_2;
		}
	}
	else if(ev < 0)
	{
		for(int8_t i = 0; i < 0-ev; i++)
		{
			f /= CUBE_ROOT_OF_2;
		}
	}
	else
	{
		return ms;
	}
	return (uint32_t) f;
}

uint8_t PTP::init()
{
	debug(STR("Initializing Camera...\r\n"));
	busy = false;
	bulb_open = false;
	currentObject = 0;
	isoAvailCount = 0;
	apertureAvailCount = 0;
	shutterAvailCount = 0;
	supports = (CameraSupports_t)
	{
		.capture = false,
		.bulb = false,
		.iso = false,
		.shutter = false,
		.aperture = false,
		.focus = false,
		.video = false,
		.cameraReady = false
	};

//	if(strncmp(PTP_CameraMake, "Canon", 5) == 0) // This should be done with VendorID instead
	if(conf.cameraMake == CANON) // This should be done with VendorID instead
	{
		debug(STR("Using Canon EOS PTP Protocol\r\n"));
	    PTP_propertyOffset = (uint16_t)(((uint8_t*)&PTP_ISO_List[0].eos) - (uint8_t *)&PTP_ISO_List[0].name[0]);
	    debug(STR("Property Offset (Canon): "));
	    debug(PTP_propertyOffset);
	    debug_nl();
	    PTP_protocol = PROTOCOL_EOS;
	}
	//else if(strncmp(PTP_CameraMake, "Nikon", 5) == 0)
	else if(conf.cameraMake == NIKON)
	{
		debug(STR("Using Nikon PTP Protocol\r\n"));
	    PTP_protocol = PROTOCOL_NIKON;
	    PTP_propertyOffset = (uint16_t)(((uint8_t*)&PTP_ISO_List[0].nikon) - (uint8_t *)&PTP_ISO_List[0].name[0]);
	    debug(STR("Property Offset (Nikon): "));
	    debug(PTP_propertyOffset);
	    debug_nl();
	}
	else
	{
	    PTP_protocol = PROTOCOL_GENERIC;
	    PTP_propertyOffset = (uint16_t)(((uint8_t*)&PTP_ISO_List[0].nikon) - (uint8_t *)&PTP_ISO_List[0].name[0]);
	}

//TEMP FIX////
//    PTP_propertyOffset = (uint16_t)(((uint8_t*)&PTP_ISO_List[0].eos) - (uint8_t *)&PTP_ISO_List[0].name[0]);
//    PTP_protocol = PROTOCOL_EOS;
//////////////

	uint8_t bulb_support = 0;//, pc_connect = 0;
    for(uint16_t i = 0; i < supportedOperationsCount; i++)
    {
    	if(PTP_protocol == PROTOCOL_EOS)
    	{
        	switch(supportedOperations[i])
	    	{
	    		case EOS_OC_MoveFocus:
	    			supports.focus = true;
	    			break;
	    		case EOS_OC_CAPTURE:
	    			supports.capture = true;
	    			break;
	    		case EOS_OC_VIDEO_START:
	    			supports.video = true;
	    			break;
//				case EOS_OC_SETUILOCK:
//				case EOS_OC_RESETUILOCK:
//				case EOS_OC_BULBSTART:
//				case EOS_OC_BULBEND:
				case EOS_OC_REMOTE_RELEASE_ON:
				case EOS_OC_REMOTE_RELEASE_OFF:
					bulb_support++;
					if(bulb_support == 2) supports.bulb = true;
					break;
				case EOS_OC_PC_CONNECT:
					debug(STR("Using PC Connect Mode\r\n"));
					//pc_connect = 1;
					break;
	    	}
		
    	}
    	else if(PTP_protocol == PROTOCOL_NIKON)
    	{
        	switch(supportedOperations[i])
	    	{
	    		case PTP_OC_CAPTURE:
	    			supports.capture = true;
	    			break;
	    		case NIKON_OC_BULBSTART:
	    		case NIKON_OC_BULBEND:
	    			supports.bulb = true;
	    			break;
	    		case PTP_OC_PROPERTY_SET:
	    			supports.iso = true;
	    			supports.aperture = true;
	    			supports.shutter = true;
	    			break;
	    		case NIKON_OC_CAMERA_READY:
	    			supports.cameraReady = true;
	    			break;
	    	}
		
    	}
    }
    if(supports.capture) debug(STR("Supports CAPTURE\r\n"));
    if(supports.bulb) debug(STR("Supports BULB\r\n"));
    if(supports.video) debug(STR("Supports VIDEO\r\n"));

    if(PTP_protocol == PROTOCOL_EOS)
    {
		data[0] = 0x00000001;
		if(PTP_Transaction(EOS_OC_PC_CONNECT, 0, 1, data, 0, NULL)) return 1; // PC Connect Mode //
		data[0] = 0x00000001;
		if(PTP_Transaction(EOS_OC_EXTENDED_EVENT_INFO_SET, 0, 1, data, 0, NULL)) return 1; // Extended Event Info Mode //
    }

	isoPTP = 0xFF;
	aperturePTP = 0xFF;
	shutterPTP = 0xFF;

	static_ready = 1;
	ready = 1;
	modeLiveView = false;
	recording = false;
	checkEvent();

	return 0;
}

uint8_t PTP::liveView(uint8_t on)
{
	if(ready == 0) return 0;
	uint8_t ret;
	if(true)
	{
		ret = setEosParameter(EOS_DPC_LiveView, (on) ? 2 : 0);
	}
	else
	{
		if(on)
			ret = PTP_Transaction(EOS_OC_LV_START, 0, 0, NULL, 0, NULL);
		else
			ret = PTP_Transaction(EOS_OC_LV_STOP, 0, 0, NULL, 0, NULL);
	}
	
	if(ret == PTP_RETURN_ERROR)
	{
		debug(STR("ERROR!\r\n"));
		return 1;	
	}
	else
	{
		modeLiveView = on;
	}

	return 0;	
}

uint8_t PTP::moveFocus(int16_t step)
{
	if(PTP_protocol == PROTOCOL_EOS)
	{
		if(!modeLiveView) // Only works in live view mode
		{
			liveView(true);
		}
		data[0] = (uint32_t) step;
		return PTP_Transaction(EOS_OC_MoveFocus, 0, 1, data, 0, NULL);
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		data[0] = (uint32_t) step > 0 ? 1: 0;
		if(step < 0) step = 0 - step;
		data[1] = (uint32_t) step;
		return PTP_Transaction(NIKON_OC_MoveFocus, 0, 2, data, 0, NULL);
	}
	return 0;
}


uint8_t PTP::checkEvent()
{
	uint8_t ret;
	uint32_t event_size;
	uint32_t event_type;
	uint32_t event_item;
	uint32_t event_value;
	uint32_t i = 0;

	if(ready == 0) return 0;
	if(bulb_open) return 0; // Because the bulb is closed asynchronously (by the clock), this prevents collisions
	
	if(PTP_protocol != PROTOCOL_EOS) // NIKON =======================================================
	{
		if(supports.cameraReady)
		{
			ret = PTP_Transaction(NIKON_OC_CAMERA_READY, 0, 0, NULL, 0, NULL);
			if(PTP_Response_Code == 0x2019)
			{
				wdt_reset();
				busy = true;
				return 0;
			}
			busy = false;
		}
		ret = PTP_Transaction(NIKON_OC_EVENT_GET, 1, 0, NULL, 0, NULL);
		uint16_t nevents, tevent;
		memcpy(&nevents, &PTP_Buffer[i], sizeof(uint16_t));
		i += sizeof(uint16_t);
		while(i < PTP_Bytes_Received)
		{
			memcpy(&tevent, &PTP_Buffer[i], sizeof(uint16_t));
			i += sizeof(uint16_t);
			memcpy(&event_value, &PTP_Buffer[i], sizeof(uint32_t));
			i += sizeof(uint32_t);
			switch(tevent)
			{
				case PTP_EC_OBJECT_CREATED:
					currentObject = event_value; // Save the object ID for later retrieving the thumbnail
					debug(STR("\r\n Object added: "));
					sendHex((char *)&currentObject);
					break;
				case PTP_EC_PROPERTY_CHANGED:
					PTP_need_update = true;
					break;
			}
		}
		if(PTP_need_update) updatePtpParameters();
		return ret;
	}

	ret = PTP_FIRST_TIME; // CANON ==================================================================
	do {
		if(ret == PTP_FIRST_TIME)
		{
			ret = PTP_Transaction(EOS_OC_EVENT_GET, 1, 0, NULL, 0, NULL);
			i = 0;
			if(ret == PTP_RETURN_ERROR)
			{
				debug(STR("ERROR checking events!\r\n"));
				return 1;	
			}
			else
			{
				if(PTP_Bytes_Received == 0)
				{
					busy = false;
				}
			}
		}
		else
		{
			debug(STR("Fetching next data packet... \r\n"));
			ret = PTP_FetchData(0);
			if(ret == PTP_RETURN_ERROR)
			{
				debug(STR("Error fetching packet!"));
				return ret;	
			}
			i = 0;
		}
		while(i < PTP_Bytes_Received)
		{
			if(i + sizeof(uint32_t) >= PTP_BUFFER_SIZE)
			{
				if(ret == PTP_RETURN_DATA_REMAINING)
				{
					debug(STR("Pre-fetching next data packet... \r\n"));
					ret = PTP_FetchData(PTP_BUFFER_SIZE - i);
					if(ret == PTP_RETURN_ERROR) return ret;
					i = 0;
					continue;
				}
				else
				{
					debug(STR("Pre-fetch Incomplete! \r\n"));
					return PTP_RETURN_ERROR;
				}
			}
			memcpy(&event_size, &PTP_Buffer[i], sizeof(uint32_t));
			if(event_size == 0)
			{
				debug(STR("ERROR: Zero-length\r\n"));
				return PTP_RETURN_ERROR;
			}
			if((event_size + i) > PTP_BUFFER_SIZE)
			{
				if(ret == PTP_RETURN_DATA_REMAINING)
				{
					if(event_size > PTP_BUFFER_SIZE)
					{
						debug(STR("Too Big: "));
						debug(event_size);
						debug_nl();
						debug(STR(" i: "));
						debug(i);
						debug_nl();
						for(uint8_t x = 0; x < 12; x++)
						{
							sendByte(PTP_Buffer[i + x - 4]);
						}
						while(event_size > PTP_BUFFER_SIZE && ret == PTP_RETURN_DATA_REMAINING)
						{
							ret = PTP_FetchData(0);
						 	event_size -= PTP_BUFFER_SIZE;
						}
						i = event_size;
						continue;
					}
					else
					{
						debug(STR("Fetching next data packet (2)... \r\n"));
						ret = PTP_FetchData(PTP_BUFFER_SIZE - i);
						if(ret == PTP_RETURN_ERROR) return ret;
						i = 0;
						continue;

					}
				}
				else
				{
					debug(STR("Incomplete! \r\n"));
					return PTP_RETURN_ERROR;
				}
			}

			memcpy(&event_type, &PTP_Buffer[i + sizeof(uint32_t) * 1], sizeof(uint32_t));
			memcpy(&event_item, &PTP_Buffer[i + sizeof(uint32_t) * 2], sizeof(uint32_t));
			memcpy(&event_value, &PTP_Buffer[i + sizeof(uint32_t) * 3], sizeof(uint32_t));

			#ifdef EXTENDED_DEBUG
			if(i > 1200)
			{
			debug(STR(" Event: "));
			debug(i);
			debug_nl();
			sendHex((char *) &event_size);
			debug(STR("        "));
			sendHex((char *) &event_type);
			debug(STR("        "));
			sendHex((char *) &event_item);
			debug(STR("        "));
			sendHex((char *) &event_value);
			debug_nl();
			}
			#endif

			if(event_type == EOS_EC_PROPERTY_CHANGE)
			{
				switch(event_item)
				{
					case EOS_DPC_ISO:
						isoPTP = event_value;
						debug(STR(" ISO:"));
						debug(event_value);
						debug_nl();
						break;
					case EOS_DPC_SHUTTER:
						shutterPTP = event_value;
						debug(STR(" SHUTTER:"));
						debug(event_value);
						debug_nl();
						break;
					case EOS_DPC_APERTURE:
						aperturePTP = event_value;
						debug(STR(" APERTURE:"));
						debug(event_value);
						debug_nl();
						break;
					case EOS_DPC_MODE:
						modePTP = event_value;
						debug(STR(" MODE:"));
						debug(event_value);
						debug_nl();
						break;
				}
			}
			else if(event_type == EOS_EC_PROPERTY_VALUES)
			{
				#ifdef EXTENDED_DEBUG
				//debug(STR(" Value List: "));
				//sendHex((char *)&event_item);
				//debug_nl();
				#endif

				uint32_t x;
				switch(event_item)
				{
					case EOS_DPC_ISO:
						for(x = 0; x < event_size / sizeof(uint32_t) - 5; x++)
						{
							isoAvail[x] = PTP::isoEv(PTP_Buffer[i+(x+5)*sizeof(uint32_t)]);
						}
						isoAvailCount = x;
						supports.iso = isoAvailCount > 0;

						#ifdef EXTENDED_DEBUG
						debug(STR("\r\n ISO Avail Count: 0x"));
						debug(isoAvailCount);
						debug_nl();
						#endif

						break;
					case EOS_DPC_SHUTTER:
						for(x = 0; x < event_size / sizeof(uint32_t) - 5; x++)
						{
							shutterAvail[x] = PTP::shutterEv(PTP_Buffer[i+(x+5)*sizeof(uint32_t)]);
						}
						shutterAvailCount = x;
						supports.shutter = shutterAvailCount > 0;
						
						#ifdef EXTENDED_DEBUG
						debug(STR("\r\n Shutter Avail Count: 0x"));
						debug(shutterAvailCount);
						debug_nl();
						#endif

						break;
					case EOS_DPC_APERTURE:
						for(x = 0; x < event_size / sizeof(uint32_t) - 5; x++)
						{
							apertureAvail[x] = PTP::apertureEv(PTP_Buffer[i+(x+5)*sizeof(uint32_t)]);
						}
						apertureAvailCount = x;
						supports.aperture = apertureAvailCount > 0;

						#ifdef EXTENDED_DEBUG
						debug(STR("\r\n Aperture Avail Count: "));
						debug(apertureAvailCount);
						debug_nl();
						#endif

						break;
					/*case EOS_DPC_MODE: // Always lists 0 since this is supposedly not writable
						for(x = 0; x < event_size / sizeof(uint32_t) - 5; x++)
						{
							apertureAvail[x] = (uint8_t) PTP_Buffer[i+(x+5)*sizeof(uint32_t)];
						}
						modeAvailCount = x;
						//supports.mode = modeAvailCount > 0;

						//#ifdef EXTENDED_DEBUG
						debug(STR("\r\n Mode Avail Count: "));
						debug(modeAvailCount);
						debug_nl();
						//#endif

						break;*/
				}
			}
			else if(event_type == EOS_EC_OBJECT_CREATED)
			{
				busy = false;

				//for(uint8_t x = 0; x < event_size / sizeof(uint32_t); x++)
				//{
				//	uint32_t tval;
				//	tval = PTP_Buffer[i+x*sizeof(uint32_t)];
				//	sendHex((char *)&tval);
				//}
				//for(uint8_t x = 0; x < event_size; x++)
				//{
				//	sendByte(PTP_Buffer[i+x]);
				//}
																		 //11
				memcpy(&currentObject, &PTP_Buffer[i + sizeof(uint32_t) * 2], sizeof(uint32_t)); // Save the object ID for later retrieving the thumbnail

				debug(STR("\r\n Object added: "));
				sendHex((char *)&currentObject);

				//getThumb(currentObject);
			}
			else if(event_type == EOS_EC_WillShutdownSoon)
			{
//				debug(STR("Keeping camera on\r\n"));
//				PTP_Transaction(EOS_OC_KeepDeviceOn, 0, 0, NULL);
			}
			else if(event_type > 0)
			{
				//debug(STR("\r\n Unknown: "));
				//sendHex((char *)&event_type);
				//debug(STR("\r\n    Size: "));
				//debug(event_size);
				//debug_nl();
				//debug(STR("\r\n  Value1: "));
				//sendHex((char *)&event_value);
				//debug(STR("\r\n  Value2: "));
				//sendHex((char *)(&event_value+4));
				//debug(STR("\r\n  Value3: "));
				//sendHex((char *)(&event_value+8));
			}
			i += event_size;
			wdt_reset();
		}
	} while(ret == PTP_RETURN_DATA_REMAINING);
	return 0;
}

void sendHex(char *hex)
{
	wdt_reset();
	for(uint8_t i = 4; i > 0; i--)
	{
		char b[4];
		b[0] = (hex[i-1] >> 4) + '0'; if(b[0] > '9') b[0] += 7;
		b[1] = (hex[i-1] & 0x0F) + '0'; if(b[1] > '9') b[1] += 7;
		b[2] = ' ';
		b[3] = 0;
		debug(b);
		if(i == 1)
		{
			b[0] = '\r';
			b[1] = '\n';
			b[2] = 0;
			debug(b);
		}
	}
}

void sendByte(char byte)
{
	wdt_reset();
	char b[4];
	b[0] = (byte >> 4) + '0'; if(b[0] > '9') b[0] += 7;
	b[1] = (byte & 0x0F) + '0'; if(b[1] > '9') b[1] += 7;
	b[2] = ' ';
	b[3] = 0;
	debug(b);
	b[0] = '\r';
	b[1] = '\n';
	b[2] = 0;
	debug(b);
}

uint8_t PTP::close()
{
	debug(STR("PTP Closed\r\n"));
	static_ready = 0;
	ready = 0;
	busy = false;
	bulb_open = false;
	return 0;
	supports = (CameraSupports_t)
	{
		.capture = false,
		.bulb = false,
		.iso = false,
		.shutter = false,
		.aperture = false,
		.focus = false,
		.video = false,
		.cameraReady = false
	};
	isoPTP = 0xFF;
	aperturePTP = 0xFF;
	shutterPTP = 0xFF;
	isoAvailCount = 0;
	apertureAvailCount = 0;
	shutterAvailCount = 0;
}

uint8_t PTP::capture()
{
	if(!static_ready) return 0;
	if(modePTP == 0x04 || preBulbShutter) manualMode();
	busy = true;
	if(PTP_protocol == PROTOCOL_EOS)
	{
		return PTP_Transaction(EOS_OC_CAPTURE, 0, 0, NULL, 0, NULL);
	}
/*	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		return PTP_Transaction(NIKON_OC_CAPTURE, 0, 0, NULL, 0, NULL);
	}*/
	else
	{
		data[0] = 0x00000000;
		data[1] = 0x00000000;
		return PTP_Transaction(PTP_OC_CAPTURE, 0, 2, data, 0, NULL);
	}
}

uint8_t PTP::videoStart()
{
	if(!static_ready) return 0;
	if(!supports.video) return PTP_RETURN_ERROR;
	if(PTP_protocol == PROTOCOL_EOS && modeLiveView)
	{
		recording = true;
		busy = true;
		setEosParameter(EOS_DPC_Video, 0x04);
	}
	return 0;
}

uint8_t PTP::videoStop()
{
	if(!static_ready) return 0;
	if(!supports.video) return PTP_RETURN_ERROR;
	if(PTP_protocol == PROTOCOL_EOS && recording)
	{
		busy = false;
		recording  = false;
		setEosParameter(EOS_DPC_Video, 0x00);
	}
	return 0;
}

uint8_t PTP::bulbMode()
{
	if(conf.modeSwitch == USB_CHANGE_MODE_DISABLED) return 0;
	if(PTP_protocol == PROTOCOL_EOS)
	{
		if(modePTP != 0x04) return setEosParameter(EOS_DPC_MODE, 0x04); // Bulb Mode
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		preBulbShutter = shutter();
		setPtpParameter(0xd100, (uint32_t)0xffffffff);
	}
	return 0;
}

uint8_t PTP::manualMode()
{
	if(conf.modeSwitch == USB_CHANGE_MODE_DISABLED) return 0;
	if(PTP_protocol == PROTOCOL_EOS)
	{
		if(modePTP != 0x03) return setEosParameter(EOS_DPC_MODE, 0x03); // Manual Mode
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		debug(STR("ManualMode: "));
		debug(preBulbShutter);
		debug_nl();
		if(preBulbShutter)
		{
			uint8_t tmp = preBulbShutter;
			preBulbShutter = 0;
			setShutter(tmp);
		}
	}
	return 0;
}

uint8_t PTP::bulbStart()
{
	if(!static_ready) return 0;
	if(bulb_open) return 1;
	if(!supports.bulb) return 1;
	bulb_open = true;
	busy = true;
	bulbMode();
	if(PTP_protocol == PROTOCOL_EOS)
	{
		data[0] = 0x03;
		data[1] = 0x00;
//		if(PTP_Transaction(EOS_OC_SETUILOCK, 0, 0, NULL, 0, NULL)) return 1; // SetUILock
//		if(PTP_Transaction(EOS_OC_BULBSTART, 0, 0, NULL, 0, NULL)) return 1; // Bulb Start
		if(PTP_Transaction(EOS_OC_REMOTE_RELEASE_ON, 0, 2, data, 0, NULL)) return 1; // Bulb Start
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		data[0] = 0xFFFFFFFF;
		data[1] = 0x00000001;
		if(PTP_Transaction(NIKON_OC_BULBSTART, 0, 2, data, 0, NULL)) return 1; // Bulb Start
	}
	return 0;
}

uint8_t PTP::bulbEnd()
{
	if(!static_ready) return 0;
	if(bulb_open == false) return 1;
	if(!supports.bulb) return 1;
	busy = true;
	if(PTP_protocol == PROTOCOL_EOS)
	{
		data[0] = 0x03;
//		if(PTP_Transaction(EOS_OC_BULBEND, 0, 0, NULL, 0, NULL)) return 1; // Bulb End
//		if(PTP_Transaction(EOS_OC_RESETUILOCK, 0, 0, NULL, 0, NULL)) return 1; // ResetUILock
		if(PTP_Transaction(EOS_OC_REMOTE_RELEASE_OFF, 0, 1, data, 0, NULL)) return 1; // Bulb End
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		data[0] = 0x0000D800;
		data[1] = 0x00000001; // This parameter varies (0x01, 0x21) -- I don't know what it means
		if(PTP_Transaction(NIKON_OC_BULBEND, 0, 2, data, 0, NULL)) return 1; // Bulb End
	}
	bulb_open = false;
	return 0;
}

uint8_t PTP::setISO(uint8_t ev)
{
	if(ev == 0xff) return 1;
	if(PTP_protocol == PROTOCOL_EOS)
	{
		return setEosParameter(EOS_DPC_ISO, isoEvPTP(ev));
	}
	else
	{
		return setPtpParameter(NIKON_DPC_ISO, (uint16_t)isoEvPTP(ev));
	}
}

uint8_t PTP::setShutter(uint8_t ev)
{
	if(ev == 0xff) return 1;
	manualMode();
	if(PTP_protocol == PROTOCOL_EOS)
	{
		return setEosParameter(EOS_DPC_SHUTTER, shutterEvPTP(ev));
	}
	else
	{
		return setPtpParameter(NIKON_DPC_SHUTTER, (uint32_t)shutterEvPTP(ev));
	}
}

uint8_t PTP::setAperture(uint8_t ev)
{
	if(ev == 0xff) return 1;
	if(PTP_protocol == PROTOCOL_EOS)
	{
		return setEosParameter(EOS_DPC_APERTURE, apertureEvPTP(ev));
	}
	else
	{
		return setPtpParameter(NIKON_DPC_APERTURE, (uint16_t)apertureEvPTP(ev));
	}
}

uint8_t PTP::setEosParameter(uint16_t param, uint32_t value)
{
	shutter_off_quick(); // Can't set parameters while half-pressed
	data[0] = 0x0000000C;
	data[1] = (uint32_t) param;
	data[2] = (uint32_t) value;
	return PTP_Transaction(EOS_OC_PROPERTY_SET, 0, 0, NULL, sizeof(uint32_t) * 3, (uint8_t*)data);
}

uint8_t PTP::setPtpParameter(uint16_t param, uint32_t value)
{
	PTP_need_update = 1;
	data[0] = (uint32_t)param;
	shutter_off_quick(); // Can't set parameters while half-pressed
	return PTP_Transaction(PTP_OC_PROPERTY_SET, 1, 1, data, sizeof(value), (uint8_t*) &value);
}
uint8_t PTP::setPtpParameter(uint16_t param, uint16_t value)
{
	PTP_need_update = 1;
	data[0] = (uint32_t)param;
	shutter_off_quick(); // Can't set parameters while half-pressed
	return PTP_Transaction(PTP_OC_PROPERTY_SET, 1, 1, data, sizeof(value), (uint8_t*) &value);
}
uint8_t PTP::setPtpParameter(uint16_t param, uint8_t value)
{
	PTP_need_update = 1;
	data[0] = (uint32_t)param;
	shutter_off_quick(); // Can't set parameters while half-pressed
	return PTP_Transaction(PTP_OC_PROPERTY_SET, 1, 1, data, sizeof(value), (uint8_t*) &value);
}

uint8_t PTP::getPtpParameterList(uint16_t param, uint8_t *count, uint16_t *list, uint16_t *current)
{
	uint8_t cnt;
	data[0] = (uint32_t)param;
	uint8_t ret = PTP_Transaction(PTP_OC_PROPERTY_LIST, 1, 1, data, 0, NULL);
	if(PTP_Bytes_Received > 10)
	{
		cnt = (uint8_t)PTP_Buffer[10];
		memcpy(current, &PTP_Buffer[7], sizeof(uint16_t));
		for(uint8_t i = 0; i < cnt; i++)
		{
			memcpy(&list[i], &PTP_Buffer[12 + i * sizeof(uint16_t)], sizeof(uint16_t));
		}
		*count = cnt;
	}
	return ret;
}

uint8_t PTP::getPtpParameterList(uint16_t param, uint8_t *count, uint32_t *list, uint32_t *current)
{
	uint8_t cnt;
	data[0] = (uint32_t)param;
	uint8_t ret = PTP_Transaction(PTP_OC_PROPERTY_LIST, 1, 1, data, 0, NULL);
	if(PTP_Bytes_Received > 10)
	{
		cnt = (uint8_t)PTP_Buffer[14];
		memcpy(current, &PTP_Buffer[9], sizeof(uint32_t));
		for(uint8_t i = 0; i < cnt; i++)
		{
			memcpy(&list[i], &PTP_Buffer[16 + i * sizeof(uint32_t)], sizeof(uint32_t));
		}
		*count = cnt;
	}
	return ret;
}

uint8_t PTP::getPtpParameter(uint16_t param, uint16_t *value)
{
	data[0] = (uint32_t)param;
	uint8_t ret = PTP_Transaction(PTP_OC_PROPERTY_GET, 1, 1, data, 0, NULL);
	if(PTP_Bytes_Received == sizeof(uint16_t)) *value = (uint16_t)PTP_Buffer;
	return ret;
}

uint8_t PTP::updatePtpParameters(void)
{
	PTP_need_update = 0;
	return 0; // temp test
	if(PTP_protocol == PROTOCOL_NIKON)
	{
		/*
		getPropertyInfo(NIKON_DPC_ISO, sizeof(uint16_t), (uint16_t*)&isoAvailCount, (uint8_t*)&isoPTP, (uint8_t*)&isoAvail);
		if(isoAvailCount > 0) supports.iso = true; else supports.iso = false;
		getPropertyInfo(NIKON_DPC_APERTURE, sizeof(uint16_t), (uint16_t*)&apertureAvailCount, (uint8_t*)&aperturePTP, (uint8_t*)&apertureAvail);
		if(apertureAvailCount > 0) supports.aperture = true; else supports.aperture = false;
		getPropertyInfo(NIKON_DPC_SHUTTER, sizeof(uint32_t), (uint16_t*)&shutterAvailCount, (uint8_t*)&shutterPTP, (uint8_t*)&shutterAvail);
		if(shutterAvailCount > 0) supports.shutter = true; else supports.shutter = false;
		*/
		
		data[0] = (uint32_t)NIKON_DPC_ISO;
		PTP_Transaction(PTP_OC_PROPERTY_LIST, 1, 1, data, 0, NULL);
		if(PTP_Bytes_Received > 10 && PTP_Buffer[2] == 4)
		{
			isoAvailCount = (uint8_t)PTP_Buffer[10];
			if(isoAvailCount > 0) supports.iso = true; else supports.iso = false;
			uint16_t tmp16;
			memcpy(&tmp16, &PTP_Buffer[7], sizeof(uint16_t));
			isoPTP = tmp16;
			for(uint8_t i = 0; i < isoAvailCount; i++)
			{
				if(i >= 32) break;
				memcpy(&tmp16, &PTP_Buffer[12 + i * sizeof(uint16_t)], sizeof(uint16_t));
				isoAvail[i] = PTP::isoEv((uint32_t)tmp16);
			}
		}

		data[0] = (uint32_t)NIKON_DPC_APERTURE;
		PTP_Transaction(PTP_OC_PROPERTY_LIST, 1, 1, data, 0, NULL);
		if(PTP_Bytes_Received > 10 && PTP_Buffer[2] == 4)
		{
			apertureAvailCount = (uint8_t)PTP_Buffer[10];
			if(apertureAvailCount > 0) supports.aperture = true; else supports.aperture = false;
			uint16_t tmp16;
			memcpy(&tmp16, &PTP_Buffer[7], sizeof(uint16_t));
			aperturePTP = tmp16;
			for(uint8_t i = 0; i < apertureAvailCount; i++)
			{
				if(i >= 32) break;
				memcpy(&tmp16, &PTP_Buffer[12 + i * sizeof(uint16_t)], sizeof(uint16_t));
				apertureAvail[i] = PTP::apertureEv((uint32_t)tmp16);
			}
		}

		data[0] = (uint32_t)NIKON_DPC_SHUTTER;
		PTP_Transaction(PTP_OC_PROPERTY_LIST, 1, 1, data, 0, NULL);
		if(PTP_Bytes_Received > 14 && PTP_Buffer[2] == 6)
		{
			shutterAvailCount = (uint8_t)PTP_Buffer[14];
			if(shutterAvailCount > 0) supports.shutter = true; else supports.shutter = false;
			memcpy(&shutterPTP, &PTP_Buffer[9], sizeof(uint32_t));
			for(uint8_t i = 0; i < shutterAvailCount; i++)
			{
				if(i >= 64) break;
				uint32_t tmp32;
				memcpy(&tmp32, &PTP_Buffer[16 + i * sizeof(uint32_t)], sizeof(uint32_t));
				shutterAvail[i] = PTP::shutterEv(tmp32);
			}
		}
		
	}
	return 0;
}

	// This method still needs work... //
uint8_t PTP::getPropertyInfo(uint16_t prop_code, uint8_t expected_size, uint16_t *count, uint8_t *current, uint8_t *list)
{

	//First check that the prop_code is in the list of supported properties (not implemented)

	// Send a command to the device to describe this property.
	data[0] = (uint32_t)prop_code;
	PTP_Transaction(PTP_OC_PROPERTY_LIST, 1, 1, data, 0, NULL);

	// data[0]
	// data[1] -- Property code back
	uint16_t prop;
	memcpy(&prop, &PTP_Buffer[0], sizeof(uint16_t));
    if(prop != prop_code) return PTP_RETURN_ERROR;

	debug(STR("PROP="));
	debug(prop);
	debug_nl();

	// data[2]
	// data[3] -- data type code
	// Setting the type code for the property also turns its
	// support flag on.
    uint16_t type = 0;
	memcpy(&type, &PTP_Buffer[2], sizeof(uint16_t));

	debug(STR("TYPE="));
	debug(type);
	debug_nl();

	// data[4] -- GetSet flag
    //uint8_t getset = PTP_Buffer[4];

    uint16_t index = 5;
	// Starting at data[5]...
	//   -- Factory Default value
	//   -- Current value
      switch (type) {
	  case 0:  // UNDEFINED
	  case 1:  // INT8
	  case 2:  // UINT8
	  	if(expected_size != sizeof(uint8_t)) return PTP_RETURN_ERROR;
		memcpy(current, &PTP_Buffer[index], sizeof(uint8_t));
		index += sizeof(uint8_t) * 2;
	    break;
	  case 3:  // INT16
	  case 4:  // UINT16
	  	if(expected_size != sizeof(uint16_t)) return PTP_RETURN_ERROR;
		memcpy(current, &PTP_Buffer[index], sizeof(uint16_t));
		index += sizeof(uint16_t) * 2;
	    break;
	  case 5:  // INT32
	  case 6:  // UINT32
	  	if(expected_size != sizeof(uint32_t)) return PTP_RETURN_ERROR;
		memcpy(current, &PTP_Buffer[index], sizeof(uint32_t));
		index += sizeof(uint32_t) * 2;
	    break;
	  case 7:  // INT64
	  case 8:  // UINT64
	  case 9:  // INT128;
	  case 10: // UINT128;
	  	return PTP_RETURN_ERROR;
	    break;
	  case 0xffff: // String
	  	return PTP_RETURN_ERROR;
	    break;
	  default:
	  	return PTP_RETURN_ERROR;
	    break;
      }

	debug(STR("CURRENT="));
	debug(*((uint16_t*)current));
	debug_nl();

	// The form flag...
    uint8_t form = PTP_Buffer[index];
    index++;

	debug(STR("FORM="));
	debug(form);
	debug_nl();

      if (form == 1) { // RANGE
	      // The range description includes 3 values: the minimum
	      // value, the maximum value and the step.
	    switch (type) {
		case 1:  // INT8
		case 2: { // UINT8
			uint8_t *range = (uint8_t*)list;
		    range[0] = PTP_Buffer[index];
			index++;
		    range[1] = PTP_Buffer[index];
			index++;
		    range[2] = PTP_Buffer[index];
			index++;
		      break;
		}
		default: {
		      break;
		}
	    }

      } else if (form == 2) { // ENUM
	      // An enumeration is a complete list of the possible
	      // value that the property can take.
		uint16_t enum_count;
		memcpy(&enum_count, &PTP_Buffer[index], sizeof(uint16_t));
		index += sizeof(uint16_t);
		*count = enum_count;

		debug(STR("COUNT="));
		debug(*count);
		debug_nl();


	    switch (type) {
		case 1: // INT8
		case 2: // UINT8
		{
		  for (uint16_t idx = 0 ; idx < enum_count ; idx++)
		  {
			memcpy(&list[idx * sizeof(uint8_t)], &PTP_Buffer[index], sizeof(uint8_t));
			index += sizeof(uint8_t);
		  }
		  break;
		}
		case 3: // INT16
		case 4: // UINT16
		{
		  for (uint16_t idx = 0 ; idx < enum_count ; idx++)
		  {
			memcpy(&list[idx * sizeof(uint16_t)], &PTP_Buffer[index], sizeof(uint16_t));
			index += sizeof(uint16_t);
		  }
		  break;
		}
		case 5: // INT32
		case 6: // UINT32
		{
		  for (uint16_t idx = 0 ; idx < enum_count ; idx++)
		  {
			memcpy(&list[idx * sizeof(uint32_t)], &PTP_Buffer[index], sizeof(uint32_t));
			index += sizeof(uint32_t);
		  }
		  break;
		}
		case 0xffff: // String
		  break;
		default:
		  break;
	    }
      } else {
      }

      return 0;
}

uint8_t PTP::getCurrentThumbStart()
{
	if(!currentObject) return 0;
	return getThumb(currentObject);
}
uint8_t PTP::getCurrentThumbContinued()
{
	uint8_t ret = PTP_FetchData(0);
	if(ret == PTP_RETURN_ERROR)
	{
		debug(STR("Error Retrieving thumbnail (c)!\r\n"));
		return 0;
	}
	return ret;
}
uint8_t PTP::getThumb(uint32_t handle)
{
	data[0] = handle;
	uint8_t ret = PTP_Transaction(PTP_OC_GET_THUMB, 1, 1, data, 0, NULL);
	if(ret == PTP_RETURN_ERROR)
	{
		debug(STR("Error Retrieving thumbnail!\r\n"));
		return 0;
	}
	else
	{
		debug(STR("Obj Size: "));
		debug(PTP_Bytes_Total);
		debug_nl();
		return ret;
	}
}

uint8_t PTP::writeFile(char *name, uint8_t *data, uint16_t dataSize)
{
	uint32_t parent = 0x0;
	uint32_t storage = 0x0;

    struct ptp_object_info objectinfo;
    memset(&objectinfo,0,sizeof(objectinfo));

    for(uint8_t i = 0; i < sizeof(objectinfo.filename); i += 2)
    {
    	objectinfo.filename[i] = name[i / 2];
    	if(name[i / 2] == 0) break;
    }

    objectinfo.object_format = PTP_OFC_Text;
	sendObjectInfo(&storage, &parent, &objectinfo);
	sendObject(data, dataSize);
	return 0;
}

uint8_t PTP::sendObjectInfo(uint32_t *storage, uint32_t *parent, ptp_object_info *objectinfo)
{
	data[0] = *storage;
	data[1] = *parent;
	return PTP_Transaction(PTP_OC_SendObjectInfo, 1, 2, data, sizeof(objectinfo), (uint8_t *)objectinfo);
}

uint8_t PTP::sendObject(uint8_t *data, uint16_t dataSize)
{
	return PTP_Transaction(PTP_OC_SendObject, 0, 0, NULL, dataSize, data);
}

uint32_t pgm_read_u32(const void *addr)
{
	uint32_t val;
	uint8_t i;
	uint8_t *p, *c;
	p = (uint8_t*)&val;
	c = (uint8_t*)addr;
	for(i = 0; i < sizeof(uint32_t); i++)
	{
		p[i] = pgm_read_byte(c);
		c++;
	}
	return val;
}

