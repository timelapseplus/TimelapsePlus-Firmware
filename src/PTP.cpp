#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include "PTP_Driver.h"
#include "PTP.h"
#include "PTP_Codes.h"
#include "shutter.h"
#include "tldefs.h"
#include "debug.h"
#ifdef PTP_DEBUG
#include "bluetooth.h"
#endif

const propertyDescription_t PTP_Aperture_List[] PROGMEM = {
    {"  Error", 0xFF, 0xFF, 255 },
    {"  f/1.2", 0x0c, 0x00000000, 2 },
    {"  f/1.4", 0x10, 0x00000000, 3 },
    {"  f/1.6", 0x13, 0x00000000, 4 },
    {"  f/1.8", 0x14, 0x000000b4, 5 },
    {"  f/2.0", 0x18, 0x000000c8, 6 },
    {"  f/2.2", 0x1b, 0x000000dc, 7 },
    {"  f/2.5", 0x1c, 0x000000fa, 8 },
    {"  f/2.8", 0x20, 0x00000118, 9 },
    {"  f/3.2", 0x23, 0x00000140, 10 },
    {"  f/3.5", 0x25, 0x0000015e, 11 },
    {"  f/4.0", 0x28, 0x00000190, 12 },
    {"  f/4.5", 0x2b, 0x000001c2, 13 },
    {"  f/5.0", 0x2d, 0x000001f4, 14 },
    {"  f/5.6", 0x30, 0x00000230, 15 },
    {"  f/6.3", 0x33, 0x00000276, 16 },
    {"  f/7.1", 0x35, 0x000002c6, 17 },
    {"  f/8.0", 0x38, 0x00000320, 18 },
    {"  f/9.0", 0x3b, 0x00000384, 19 },
    {"   f/10", 0x3d, 0x000003e8, 20 },
    {"   f/11", 0x40, 0x0000044c, 21 },
    {"   f/13", 0x43, 0x00000514, 22 },
    {"   f/14", 0x45, 0x00000578, 23 },
    {"   f/16", 0x48, 0x00000640, 24 },
    {"   f/18", 0x4b, 0x00000708, 25 },
    {"   f/20", 0x4d, 0x000007d0, 26 },
    {"   f/22", 0x50, 0x00000898, 27 }
};

const propertyDescription_t PTP_Shutter_List[] PROGMEM = {
    {"  Error", 0xFF, 0x000000FF, 255 },
    {" 1/8000", 0xa0, 0x00000000, 76 },
    {" 1/6400", 0x9d, 0x00000000, 75 },
    {" 1/5000", 0x9b, 0x00000000, 74 },
    {" 1/4000", 0x98, 0x00000002, 73 },
    {" 1/3200", 0x95, 0x00000003, 72 },
    {" 1/2500", 0x93, 0x00000004, 71 },
    {" 1/2000", 0x90, 0x00000005, 70 },
    {" 1/1600", 0x8d, 0x00000006, 69 },
    {" 1/1250", 0x8b, 0x00000008, 68 },
    {" 1/1000", 0x88, 0x0000000a, 67 },
    {"  1/800", 0x85, 0x0000000c, 66 },
    {"  1/640", 0x83, 0x0000000f, 65 },
    {"  1/500", 0x80, 0x00000014, 64 },
    {"  1/400", 0x7d, 0x00000019, 63 },
    {"  1/320", 0x7b, 0x0000001f, 62 },
    {"  1/250", 0x78, 0x00000028, 61 },
    {"  1/200", 0x75, 0x00000032, 60 },
    {"  1/160", 0x73, 0x0000003e, 59 },
    {"  1/125", 0x70, 0x00000050, 58 },
    {"  1/100", 0x6d, 0x00000064, 57 },
    {"   1/80", 0x6b, 0x0000007d, 56 },
    {"   1/60", 0x68, 0x000000a6, 55 },
    {"   1/50", 0x65, 0x000000c8, 54 },
    {"   1/40", 0x63, 0x000000fa, 53 },
    {"   1/30", 0x60, 0x0000014d, 52 },
    {"   1/25", 0x5d, 0x00000190, 51 },
    {"   1/20", 0x5b, 0x000001f4, 50 },
    {"   1/15", 0x58, 0x0000029a, 49 },
    {"   1/13", 0x55, 0x00000301, 48 },
    {"   1/10", 0x53, 0x000003e8, 47 },
    {"    1/8", 0x50, 0x000004e2, 46 },
    {"    1/6", 0x4d, 0x00000682, 45 },
    {"    1/5", 0x4b, 0x000007d0, 44 },
    {"    1/4", 0x48, 0x000009c4, 43 },
    {"    0.3", 0x45, 0x00000d05, 42 },
    {"    0.4", 0x43, 0x00000fa0, 41 },
    {"    0.5", 0x40, 0x00001388, 40 },
    {"    0.6", 0x3d, 0x0000186a, 39 },
    {"    0.8", 0x3b, 0x00001e0c, 38 },
    {"     1s", 0x38, 0x00002710, 37 },
    {"   1.3s", 0x35, 0x000032c8, 36 },
    {"   1.6s", 0x33, 0x00003e80, 35 },
    {"     2s", 0x30, 0x00004e20, 34 },
    {"   2.5s", 0x2d, 0x000061a8, 33 },
    {"   3.2s", 0x2b, 0x00007530, 32 },
    {"     4s", 0x28, 0x00009c40, 31 },
    {"     5s", 0x25, 0x0000c350, 30 },
    {"     6s", 0x23, 0x0000ea60, 29 },
    {"     8s", 0x20, 0x00013880, 28 },
    {"    10s", 0x1d, 0x000186a0, 27 },
    {"    13s", 0x1b, 0x0001fbd0, 26 },
    {"    15s", 0x18, 0x000249f0, 25 },
    {"    20s", 0x15, 0x00030d40, 24 },
    {"    25s", 0x13, 0x0003d090, 23 },
    {"    30s", 0x10, 0x000493e0, 22 },
    {"   Bulb", 0x0c, 0x00000000, 254 }
};

const propertyDescription_t PTP_ISO_List[] PROGMEM = {
    {"  Error", 0xFF, 0x000000FF, 255 },
    {"   Auto", 0x00, 0x00000000, 254 },
    {"     50", 0x40, 0x00000000, 46 },
    {"    100", 0x48, 0x00000064, 43 },
    {"    125", 0x4b, 0x0000007d, 42 },
    {"    160", 0x4d, 0x000000a0, 41 },
    {"    200", 0x50, 0x000000c8, 40 },
    {"    250", 0x53, 0x000000fa, 39 },
    {"    320", 0x55, 0x00000140, 38 },
    {"    400", 0x58, 0x00000190, 37 },
    {"    500", 0x5b, 0x000001f4, 36 },
    {"    640", 0x5d, 0x00000280, 35 },
    {"    800", 0x60, 0x00000320, 34 },
    {"   1000", 0x63, 0x000003e8, 33 },
    {"   1250", 0x65, 0x000004e2, 32 },
    {"   1600", 0x68, 0x00000640, 31 },
    {"   2000", 0x6B, 0x00001f40, 30 },
    {"   2500", 0x6D, 0x00002710, 29 },
    {"   3200", 0x70, 0x00003200, 28 },
    {"   4000", 0x73, 0x00000000, 27 },
    {"   5000", 0x75, 0x00000000, 26 },
    {"   6400", 0x78, 0x00000000, 25 },
    {"  12800", 0x80, 0x00000000, 22 },
    {"  25600", 0x88, 0x00000000, 19 },
    {"  51200", 0x90, 0x00000000, 16 },
    {" 102400", 0x98, 0x00000000, 13 },
    {" 204800", 0xA0, 0x00000000, 10 }
}; 

//4294967296
//419430400

const bulbSettings_t Bulb_List[] PROGMEM = {
    {" Camera", 0,  254 },
    {"   1/20", 49, 50 },
    {"   1/15", 62, 49 },
    {"   1/13", 79, 48 },
    {"   1/10", 99, 47 },
    {"    1/8", 125, 46 },
    {"    1/6", 157, 45 },
    {"    1/5", 198, 44 },
    {"    1/4", 250, 43 },
    {"    0.3", 315, 42 },
    {"    0.4", 397, 41 },
    {"    0.5", 500, 40 },
    {"    0.6", 630, 39 },
    {"    0.8", 794, 38 },
    {"     1s", 1000, 37 },
    {"   1.3s", 1260, 36 },
    {"   1.6s", 1587, 35 },
    {"     2s", 2000, 34 },
    {"   2.5s", 2520, 33 },
    {"   3.2s", 3175, 32 },
    {"     4s", 4000, 31 },
    {"     5s", 5040, 30 },
    {"     6s", 6349, 29 },
    {"     8s", 8000, 28 },
    {"    10s", 10079, 27 },
    {"    13s", 12699, 26 },
    {"    15s", 16000, 25 },
    {"    20s", 20158, 24 },
    {"    25s", 25398, 23 },
    {"    30s", 32000, 22 },
    {"    40s", 40320, 21 },
    {"    50s", 50800, 20 },
    {"    60s", 64000, 19 },
    {"    80s", 80630, 18 },
    {"   100s", 101590, 17 },
    {"   125s", 128000, 16 },
    {"   160s", 161270, 15 },
    {"   200s", 203180, 14 },
    {"     4m", 256000, 13 },
    {"   5.3m", 322530, 12 },
    {"   6.6m", 406360, 11 },
    {"     8m", 512000, 10 },
    {"    11m", 645070, 9 },
    {"    13m", 812720, 8 },
    {"    15m", 1024000, 7 }
//    {"    20m", 1290138, 6 },
//    {"    25m", 1625444, 5 },
//    {"    30m", 2048000, 4 }
};


#ifdef PTP_DEBUG
extern BT bt;
#endif

uint8_t isoAvail[32];
uint8_t isoAvailCount;
uint8_t shutterAvail[64];
uint8_t shutterAvailCount;
uint8_t apertureAvail[32];
uint8_t apertureAvailCount;

uint32_t isoPTP;
uint32_t shutterPTP;
uint32_t aperturePTP;
uint32_t modePTP;

uint8_t preBulbMode;

uint8_t static_ready;

uint8_t PTP_protocol;
uint16_t PTP_propertyOffset;

PTP::PTP(void)
{
	static_ready = 0;
	ready = 0;
	isoAvailCount = 0;
	apertureAvailCount = 0;
	shutterAvailCount = 0;
	PTP_protocol = 0;
}

uint8_t PTP::bulbMax() // 7
{
	return pgm_read_byte(&Bulb_List[sizeof(Bulb_List) / sizeof(Bulb_List[0]) - 1].ev);
}

uint8_t PTP::bulbMin() // 47
{
	return pgm_read_byte(&Bulb_List[1].ev);
}

uint8_t PTP::isoMax() // 13
{
	if(isoAvailCount > 0)
	{
		for(uint8_t i = 1; i <= isoAvailCount; i++)
		{
			uint8_t tmp = isoAvail[isoAvailCount - i];
			if(tmp > 0 && tmp < 128) return tmp;
		}
	}
	return 0;
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
	if(PTP::shutterType(ev))
	{
		if(static_ready == 0)
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
			return pgm_read_byte(&Bulb_List[1].ev);
		}
	}
}

uint8_t PTP::shutterDown(uint8_t ev)
{
	if(shutterAvailCount > 0)
	{
		for(uint8_t i = 0; i < shutterAvailCount; i++)
		{
			if(shutterAvail[i] == ev - 1)
			{
				return shutterAvail[i];
			}
		}
	}
	for(uint8_t i = 0; i < sizeof(Bulb_List) / sizeof(Bulb_List[0]); i++)
	{
		if(pgm_read_byte(&Bulb_List[i].ev) == (ev - 1))
		{
			return pgm_read_byte(&Bulb_List[i].ev);
		}
	}
	if(ev == 0) ev = pgm_read_byte(&Bulb_List[1].ev);
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
		if(pgm_read_u32((uint32_t*)&PTP_ISO_List[i] + PTP_propertyOffset) == id)
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
		if(pgm_read_u32((uint32_t*)&PTP_Shutter_List[i] + PTP_propertyOffset) == id)
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
		if(pgm_read_u32((uint32_t*)&PTP_Aperture_List[i] + PTP_propertyOffset) == id)
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
			return pgm_read_u32((uint32_t*)&PTP_ISO_List[i] + PTP_propertyOffset);
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
			return pgm_read_u32((uint32_t*)&PTP_Shutter_List[i] + PTP_propertyOffset);
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
			return pgm_read_u32((uint32_t*)&PTP_Aperture_List[i] + PTP_propertyOffset);
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
			ret |= SHUTTER_MODE_BULB;
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
		int8_t diff = bulbMin() - ev;
		return shiftBulb(bulbTime((int8_t)bulbMax()), diff);
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
	busy = false;
	bulb_open = false;
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
		.focus = false
	};

	if(strncmp(PTP_CameraMake, "Canon", 5) == 0) // This should be done with VendorID instead
	{
		debug(STR("Using Canon EOS PTP Protocol\r\n"));
	    PTP_propertyOffset = (&PTP_ISO_List[0].eos) - (uint32_t *)&PTP_ISO_List[0];
	    PTP_protocol = PROTOCOL_EOS;
	}
	else if(strncmp(PTP_CameraMake, "Nikon", 5) == 0)
	{
		debug(STR("Using Nikon PTP Protocol\r\n"));
	    PTP_protocol = PROTOCOL_NIKON;
	    PTP_propertyOffset = (&PTP_ISO_List[0].nikon) - (uint32_t *)&PTP_ISO_List[0];
	}
	else
	{
	    PTP_protocol = PROTOCOL_GENERIC;
	    PTP_propertyOffset = (&PTP_ISO_List[0].eos) - (uint32_t *)&PTP_ISO_List[0];
	}

	uint8_t bulb_support = 0;
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
				case EOS_OC_SETUILOCK:
				case EOS_OC_RESETUILOCK:
				case EOS_OC_BULBSTART:
				case EOS_OC_BULBEND:
					bulb_support++;
					if(bulb_support == 4) supports.bulb = true;
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
	    		case PTP_OC_PROPERTY_SET:
	    			supports.iso = true;
	    			supports.aperture = true;
	    			supports.shutter = true;
	    			break;
	    	}
		
    	}
    }
    if(supports.bulb) debug(STR("Supports CAPTURE\r\n"));
    if(supports.bulb) debug(STR("Supports BULB\r\n"));

    if(PTP_protocol == PROTOCOL_EOS)
    {
		data[0] = 0x00000001;
		if(PTP_Transaction(EOS_OC_PC_CONNECT, 0, sizeof(uint32_t), data, 0, NULL)) return 1; // PC Connect Mode //
		data[0] = 0x00000001;
		if(PTP_Transaction(EOS_OC_EXTENDED_EVENT_INFO_SET, 0, sizeof(uint32_t), data, 0, NULL)) return 1; // Extended Event Info Mode //
    }

	isoPTP = 0xFF;
	aperturePTP = 0xFF;
	shutterPTP = 0xFF;

	static_ready = 1;
	ready = 1;
	liveViewOn = false;
	checkEvent();

	return 0;
}

uint8_t PTP::liveView(uint8_t on)
{
	if(ready == 0) return 0;
	uint8_t ret = setEosParameter(EOS_DPC_LiveView, (on) ? 2 : 0);
	if(ret == PTP_RETURN_ERROR)
	{
		debug(STR("ERROR!\r\n"));
		return 1;	
	}
/*	if(on)
	{
		ret = setParameter(0xD1B3, 0);
		if(ret == PTP_RETURN_ERROR)
		{
			debug(STR("ERROR!\r\n"));
			return 1;	
		}
		liveViewOn = true;
		debug(STR("LiveView ON\r\n"));
	}
	else
	{
		liveViewOn = false;
		debug(STR("LiveView OFF\r\n"));
	}
*/
	return 0;	
}

uint8_t PTP::moveFocus(uint16_t step)
{
	if(!liveViewOn) // Only works in live view mode
	{
		liveView(true);
	}
	data[0] = step;
	return PTP_Transaction(EOS_OC_MoveFocus, 0, 1, data, 0, NULL);
}

uint8_t PTP::checkEvent()
{
	if(ready == 0) return 0;
	if(bulb_open) return 0; // Because the bulb is closed asynchronously (by the clock), this prevents collisions
	if(PTP_protocol != PROTOCOL_EOS) return 0; // So far only supports Canon here

	uint8_t ret;
	do {
		ret = PTP_Transaction(EOS_OC_EVENT_GET, 1, 0, NULL, 0, NULL);
		if(ret == PTP_RETURN_ERROR)
		{
			debug(STR("ERROR!\r\n"));
			return 1;	
		}
		uint32_t event_size;
		uint32_t event_type;
		uint32_t event_item;
		uint32_t event_value;
		uint32_t i = 0;
		while(i < PTP_Bytes_Received)
		{
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
						ret = PTP_FetchData(PTP_BUFFER_SIZE - i);
						if(ret == PTP_RETURN_ERROR) return ret;
						i = 0;
						continue;

					}
				}
				else
				{
					return PTP_RETURN_ERROR;
				}
			}

			memcpy(&event_type, &PTP_Buffer[i + sizeof(uint32_t) * 1], sizeof(uint32_t));
			memcpy(&event_item, &PTP_Buffer[i + sizeof(uint32_t) * 2], sizeof(uint32_t));
			memcpy(&event_value, &PTP_Buffer[i + sizeof(uint32_t) * 3], sizeof(uint32_t));
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
/*					default:
						if(!supports.iso) break;
						#ifdef PTP_DEBUG
						bt.send(STR(" UNKNOWN: "));
						sendHex((char *) &event_item);
						bt.send(STR(", "));
						sendHex((char *) &event_value);
						#endif
						break;*/
				}
			}
			else if(event_type == EOS_EC_PROPERTY_VALUES)
			{
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
						debug(STR("\r\n ISO Avail Count: 0x"));
						debug(isoAvailCount);
						debug_nl();
						break;
					case EOS_DPC_SHUTTER:
						for(x = 0; x < event_size / sizeof(uint32_t) - 5; x++)
						{
							shutterAvail[x] = PTP::shutterEv(PTP_Buffer[i+(x+5)*sizeof(uint32_t)]);
						}
						shutterAvailCount = x;
						supports.shutter = shutterAvailCount > 0;
						debug(STR("\r\n Shutter Avail Count: 0x"));
						debug(shutterAvailCount);
						debug_nl();
						break;
					case EOS_DPC_APERTURE:
						for(x = 0; x < event_size / sizeof(uint32_t) - 5; x++)
						{
							apertureAvail[x] = PTP::apertureEv(PTP_Buffer[i+(x+5)*sizeof(uint32_t)]);
						}
						apertureAvailCount = x;
						supports.aperture = apertureAvailCount > 0;
						debug(STR("\r\n Aperture Avail Count: "));
						debug(apertureAvailCount);
						debug_nl();
						break;
				}
			}
			else if(event_type == EOS_EC_OBJECT_CREATED)
			{
				//if(preBulbMode)
				//{
				//	setParameter(EOS_DPC_MODE, preBulbMode); // Bulb Mode
				//	preBulbMode = 0;
				//}
				busy = false;
				debug(STR("\r\n Photo!\r\n"));
			}
			else if(event_type == EOS_EC_WillShutdownSoon)
			{
//				debug(STR("Keeping camera on\r\n"));
//				PTP_Transaction(EOS_OC_KeepDeviceOn, 0, 0, NULL);
			}
			else if(event_type > 0)
			{
				debug(STR("\r\n Unknown: "));
				sendHex((char *)&event_type);
				debug(STR("\r\n    Size: "));
				debug(event_size);
				debug_nl();
				debug(STR("\r\n  Value1: "));
				sendHex((char *)&event_value);
				debug(STR("\r\n  Value2: "));
				sendHex((char *)(&event_value+4));
				debug(STR("\r\n  Value3: "));
				sendHex((char *)(&event_value+8));
			}
			i += event_size;
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
		.focus = false
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
	if(modePTP == 0x04) manualMode();
	busy = true;
	if(PTP_protocol == PROTOCOL_EOS)
	{
		return PTP_Transaction(EOS_OC_CAPTURE, 0, 0, NULL, 0, NULL);
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		return PTP_Transaction(NIKON_OC_CAPTURE, 0, 0, NULL, 0, NULL);
	}
	else
	{
		data[0] = 0x00000000;
		data[1] = 0x00003801;
		return PTP_Transaction(PTP_OC_CAPTURE, 0, 2, data, 0, NULL);
	}
}

uint8_t PTP::bulbMode()
{
	if(PTP_protocol == PROTOCOL_EOS)
	{
		if(modePTP != 0x04) return setEosParameter(EOS_DPC_MODE, 0x04); // Bulb Mode
	}
	else
	{

	}
	return 0;
}

uint8_t PTP::manualMode()
{
	if(PTP_protocol == PROTOCOL_EOS)
	{
		if(modePTP != 0x03) return setEosParameter(EOS_DPC_MODE, 0x03); // Manual Mode
	}
	else
	{

	}
	return 0;
}

uint8_t PTP::bulbStart()
{
	if(bulb_open) return 1;
	bulb_open = true;
	busy = true;
	preBulbMode = modePTP;
	bulbMode();
	if(PTP_protocol == PROTOCOL_EOS)
	{
		if(PTP_Transaction(EOS_OC_SETUILOCK, 0, 0, NULL, 0, NULL)) return 1; // SetUILock
		if(PTP_Transaction(EOS_OC_BULBSTART, 0, 0, NULL, 0, NULL)) return 1; // Bulb Start
	}
	else
	{

	}
	return 0;
}

uint8_t PTP::bulbEnd()
{
	if(bulb_open == false) return 1;
	busy = true;
	if(PTP_protocol == PROTOCOL_EOS)
	{
		if(PTP_Transaction(EOS_OC_BULBEND, 0, 0, NULL, 0, NULL)) return 1; // Bulb End
		if(PTP_Transaction(EOS_OC_RESETUILOCK, 0, 0, NULL, 0, NULL)) return 1; // ResetUILock
	}
	else
	{

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
	shutter_off_quick(); // Can't set parameters while half-pressed
	return PTP_Transaction(PTP_OC_PROPERTY_SET, 0, 1, (uint32_t*) &param, sizeof(value), (uint8_t*) &value);
}
uint8_t PTP::setPtpParameter(uint16_t param, uint16_t value)
{
	shutter_off_quick(); // Can't set parameters while half-pressed
	return PTP_Transaction(PTP_OC_PROPERTY_SET, 0, 1, (uint32_t*) &param, sizeof(value), (uint8_t*) &value);
}
uint8_t PTP::setPtpParameter(uint16_t param, uint8_t value)
{
	shutter_off_quick(); // Can't set parameters while half-pressed
	return PTP_Transaction(PTP_OC_PROPERTY_SET, 0, 1, (uint32_t*) &param, sizeof(value), (uint8_t*) &value);
}

uint32_t pgm_read_u32(const uint32_t *addr)
{
	uint32_t val;
	uint8_t i;

	for(i = 0; i < sizeof(uint32_t); i++)
	{
		*(&val + i) = pgm_read_byte(addr + i);
	}

	return val;
}

