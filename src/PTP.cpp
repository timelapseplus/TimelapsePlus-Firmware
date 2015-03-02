#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include "PTP_Driver.h"
#include "PTP.h"
#include "PTP_Codes.h"
#include "shutter.h"
#include "settings.h"
#include "clock.h"
#include "tldefs.h"
#include "debug.h"
#include "PTP_Lists.h"
//#define EXTENDED_DEBUG

extern settings_t conf;
extern Clock clock;

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

uint8_t lvOCmode, ptpBulbMode;
uint8_t supports_nikon_capture;

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

uint8_t PTP::bulbMax() // 4
{
	return pgm_read_byte(&Bulb_List[sizeof(Bulb_List) / sizeof(Bulb_List[0]) - 1].ev);
}

uint8_t PTP::bulbMin() // 61 (46 for IR)
{
	uint8_t tmp = pgm_read_byte(&Bulb_List[1].ev);
	if(tmp > conf.camera.bulbMin) tmp = conf.camera.bulbMin;
	return tmp;
}

uint8_t PTP::bulbMinStatic() // 61 (46 for IR)
{
	uint8_t tmp = pgm_read_byte(&Bulb_List[1].ev);
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
			tmp = apertureAvail[apertureAvailCount - i];
			if(tmp > 0 && tmp < 128) break;
		}
		if(tmp > conf.apertureMax) tmp = conf.apertureMax;
	}
	return tmp;
}

uint8_t PTP::apertureMin() // 2
{
	uint8_t tmp = 0;
	if(apertureAvailCount > 0)
	{
		for(uint8_t i = 0; i < apertureAvailCount; i++)
		{
			tmp = apertureAvail[i];
			if(tmp > 0 && tmp < 128) break;
		}
		if(tmp < conf.apertureMin) tmp = conf.apertureMin;
	}
	return tmp;
}

uint8_t PTP::apertureWideOpen()
{
	return apertureAvail[0];
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
	uint8_t tmp = isoAvail[isoAvailCount - 1];
	if(isoAvailCount == 0) return ev;
	for(uint8_t i = 0; i < isoAvailCount; i++)
	{
		if(isoAvail[i] == ev)
		{
			if(i < isoAvailCount - 1)
			{
				tmp = isoAvail[i + 1];
				break;
			}
			else
			{
				break;
			}
		}
	}
	if(tmp < conf.isoMax) tmp = conf.isoMax;
	return tmp;
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
	if(ev == BULB_EV_CODE) return 28;
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
	if(ev == BULB_EV_CODE) return 28;
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
	uint8_t tmp = apertureAvail[apertureAvailCount - 1];
	for(uint8_t i = 0; i < apertureAvailCount; i++)
	{
		if(apertureAvail[i] == ev)
		{
			if(i < apertureAvailCount - 1)
			{
				tmp = apertureAvail[i + 1];
				break;
			}
			else
			{
				break;
			}
		}
	}
	if(tmp > conf.apertureMax) tmp = conf.apertureMax;
	return tmp;
}

uint8_t PTP::apertureDown(uint8_t ev)
{
	uint8_t tmp = apertureAvail[0];
	for(uint8_t i = 0; i < apertureAvailCount; i++)
	{
		if(apertureAvail[i] == ev)
		{
			if(i > 0)
			{
				tmp = apertureAvail[i - 1];
				break;
			}
			else
			{
				break;
			}
		}
	}
	if(tmp < conf.apertureMin) tmp = conf.apertureMin;
	return tmp;
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

uint8_t PTP::bulbName(char name[8], uint32_t bulb_time)
{
	name[0] = '\0';
	if(bulb_time == 0) return 0;
	if(bulb_time < bulbTime((int8_t)bulbMin())) return shutterName(name, bulbToShutterEv(bulb_time));
	for(uint8_t i = 0; i < sizeof(Bulb_List) / sizeof(Bulb_List[0]); i++)
	{
		if(pgm_read_u32(&Bulb_List[i].ms) >= bulb_time)
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

uint8_t PTP::bulbToShutterEv(uint32_t bulb_time)
{
	if(bulb_time == 0) return 0;
	static uint8_t prev_ev = 0;
	uint8_t ev = 0, ev1 = 0, ev2 = 0;
	uint16_t delta1, delta2;
	uint32_t buf;

	for(uint8_t i = 0; i < sizeof(PTP_Shutter_List) / sizeof(PTP_Shutter_List[0]); i++)
	{
		buf = pgm_read_u32(&PTP_Shutter_List[i].nikon);
		if(buf >= bulb_time * 10)
		{
			ev1 = pgm_read_byte(&PTP_Shutter_List[i].ev);
			delta1 = buf - bulb_time * 10;
			//DEBUG(STR("DELTA: "));
			//DEBUG(delta1);
			if(prev_ev && prev_ev == ev1) delta1 /= 2; // hysteresis
			if(i < sizeof(PTP_Shutter_List) - 1)
			{
				buf = pgm_read_u32(&PTP_Shutter_List[i + 1].nikon);
				ev2 = pgm_read_byte(&PTP_Shutter_List[i + 1].ev);
				delta2 = buf - bulb_time * 10;
				//DEBUG(STR(", "));
				//DEBUG(delta2);
				if(prev_ev && prev_ev == ev2) delta2 /= 2; // hysteresis
				if(delta2 <= delta1)
				{
					//DEBUG_NL();
					ev = ev2;
					break;
				}
			}
			//DEBUG_NL();
			ev = ev1;
			break;
		}
	}

	if(ev > MAX_EXTENDED_RAMP_SHUTTER) ev = MAX_EXTENDED_RAMP_SHUTTER;
	prev_ev = ev;
	return ev;
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
	if(ev > bulbMin())
	{
		for(uint8_t i = 0; i < sizeof(PTP_Shutter_List) / sizeof(PTP_Shutter_List[0]); i++)
		{
			if(pgm_read_byte(&PTP_Shutter_List[i].ev) == ev)
			{
				char *ptr1, *ptr2;
				ptr1 = (char *) &ms;
				ptr2 = (char *) &PTP_Shutter_List[i].nikon;
				ptr1[0] = pgm_read_byte(&ptr2[0]);
				ptr1[1] = pgm_read_byte(&ptr2[1]);
				ptr1[2] = pgm_read_byte(&ptr2[2]);
				ptr1[3] = pgm_read_byte(&ptr2[3]);
				return ms / 10;
			}
		}
	}
	else
	{
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
	//DEBUG(PSTR("Initializing Camera...\r\n"));
	busy = false;
	PTP_need_update = true;
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
		.cameraReady = false,
		.event = false
	};

	if(strncmp(PTP_CameraMake, "Canon", 5) == 0) // This should be done with VendorID instead
	{
		conf.camera.cameraMake = CANON;
	}
	else if(strncmp(PTP_CameraMake, "Nikon", 5) == 0)
	{
		conf.camera.cameraMake = NIKON;
	}

	if(conf.camera.cameraMake == CANON)
	{
		//DEBUG(PSTR("Using Canon EOS PTP Protocol\r\n"));
	    PTP_propertyOffset = (uint16_t)(((uint8_t*)&PTP_ISO_List[0].eos) - (uint8_t *)&PTP_ISO_List[0].name[0]);
	    //DEBUG(PSTR("Property Offset (Canon): "));
	    //DEBUG(PTP_propertyOffset);
	    //DEBUG_NL();
	    PTP_protocol = PROTOCOL_EOS;
	}
	else if(conf.camera.cameraMake == NIKON)
	{
		//DEBUG(PSTR("Using Nikon PTP Protocol\r\n"));
	    PTP_protocol = PROTOCOL_NIKON;
	    PTP_propertyOffset = (uint16_t)(((uint8_t*)&PTP_ISO_List[0].nikon) - (uint8_t *)&PTP_ISO_List[0].name[0]);
	    //DEBUG(PSTR("Property Offset (Nikon): "));
	    //DEBUG(PTP_propertyOffset);
	    //DEBUG_NL();
	}
	else
	{
	    PTP_protocol = PROTOCOL_GENERIC;
	    PTP_propertyOffset = (uint16_t)(((uint8_t*)&PTP_ISO_List[0].nikon) - (uint8_t *)&PTP_ISO_List[0].name[0]);
	}

	videoMode = true; // overwritten if camera has video mode property

	lvOCmode = 0;
	ptpBulbMode = 0;
	uint8_t bulbSupport = 0;

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
	    		case EOS_OC_LV_START:
	    		case EOS_OC_LV_STOP:
	    			lvOCmode++;
	    			break;
				case EOS_OC_REMOTE_RELEASE_ON:
				case EOS_OC_REMOTE_RELEASE_OFF:
					bulbSupport++;
					break;
				case EOS_OC_BULBSTART:
				case EOS_OC_BULBEND:
					ptpBulbMode++;
					break;
				case EOS_OC_PC_CONNECT:
					//DEBUG(PSTR("Using PC Connect Mode\r\n"));
					break;
				case EOS_OC_EVENT_GET:
					supports.event = true;
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
	    		case NIKON_OC_CAPTURE:
	    			if(conf.camera.nikonUSB)
	    			{
						supports.capture = true;
	    				supports_nikon_capture = true;
	    			}
	    			break;
	    		//case NIKON_OC_BULBSTART:
				//	supports.bulb = true;
	    		//	break;
	    		case PTP_OC_PROPERTY_SET:
	    			supports.iso = true;
	    			supports.aperture = true;
	    			supports.shutter = true;
	    			break;
	    		case NIKON_OC_CAMERA_READY:
	    			supports.cameraReady = true;
	    			break;
				case NIKON_OC_MoveFocus:
					supports.focus = true;
					break;
				case NIKON_OC_EVENT_GET:
					supports.event = true;
					break;
	    	}
			//sendHex((char *)&supportedOperations[i]);
    	}
    }
    for(uint16_t i = 0; i < supportedPropertiesCount; i++)
    {
    	if(PTP_protocol == PROTOCOL_EOS)
    	{
        	switch(supportedProperties[i])
	    	{
	    		case EOS_DPC_LiveView:
		    		DEBUG(PSTR("Supports DPC_LiveView\r\n"));
	    			lvOCmode = 0;
	    			break;
	    	}
    	}
    	//else if(PTP_protocol == PROTOCOL_NIKON)
    	//{
        //	switch(supportedProperties[i])
	    //	{
	    //		case EOS_DPC_LiveView:
	    //			supports.video = true;
	    //			break;
	    //	}
    	//}
    }
	if(lvOCmode > 1) lvOCmode = true; else lvOCmode = false;		
    if(bulbSupport > 1) supports.bulb = true; else supports.bulb = false;		    
    if(!supports.bulb)
    {
    	if(ptpBulbMode > 1) supports.bulb = true;
    }
    else
    {
    	ptpBulbMode = 0;
    }
    
    //if(supports.capture) DEBUG(PSTR("Supports CAPTURE\r\n"));
    //if(supports.bulb) DEBUG(PSTR("Supports BULB\r\n"));
    //if(supports.video) DEBUG(PSTR("Supports VIDEO\r\n"));
    //if(supports.focus) DEBUG(PSTR("Supports FOCUS\r\n"));
    //if(supports.event) DEBUG(PSTR("Supports EVENTS\r\n"));

    if(PTP_protocol == PROTOCOL_EOS)
    {
		data[0] = 0x00000001;
		if(PTP_Transaction(EOS_OC_PC_CONNECT, 0, 1, data, 0, NULL)) return PTP_RETURN_ERROR; // PC Connect Mode //
		data[0] = 0x00000001;
		if(PTP_Transaction(EOS_OC_EXTENDED_EVENT_INFO_SET, 0, 1, data, 0, NULL)) return PTP_RETURN_ERROR; // Extended Event Info Mode //
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
	uint8_t ret = 0;
	if(PTP_protocol == PROTOCOL_EOS)
	{
		if(lvOCmode)
		{
			//DEBUG(PSTR("Using LV OC mode\r\n"));
			if(on)
				ret = PTP_Transaction(EOS_OC_LV_START, 0, 0, NULL, 0, NULL);
			else
				ret = PTP_Transaction(EOS_OC_LV_STOP, 0, 0, NULL, 0, NULL);
		}
		else
		{
			//DEBUG(PSTR("Using LV property mode\r\n"));
			ret = setEosParameter(EOS_DPC_LiveView, (on) ? 2 : 0);
		}
		
		if(on) setEosParameter(EOS_DPC_LiveViewShow, 0);
		//if(on) setEosParameter(EOS_DPC_Video, 0x03);
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		if(on)
			ret = PTP_Transaction(NIKON_OC_StartLiveView, 0, 0, NULL, 0, NULL);
		else
			ret = PTP_Transaction(NIKON_OC_EndLiveView, 0, 0, NULL, 0, NULL);
	}

	if(ret == PTP_RETURN_ERROR)
	{
		//DEBUG(PSTR("ERROR!\r\n"));
		return PTP_RETURN_ERROR;	
	}
	else
	{
		modeLiveView = on;
	}
	return 0;	
}

// move can be -3 to +3 (step size & direction)
uint8_t PTP::moveFocus(int8_t move, uint16_t steps)
{
	if(move == 0 || steps == 0) return 0;
	uint8_t ret = 0;
	if(PTP_protocol == PROTOCOL_EOS)
	{
		if(!modeLiveView) // Only works in live view mode
		{
			return 1;
			//liveView(true);
		}
		if(move > 0)
		{
			data[0] = 0;
		}
		else
		{
			data[0] = 0x8000;
			move = 0 - move;
		}
		if(move > 3) move = 3;
		data[0] += move;

		while(steps && !ret)
		{
			if(!ret) ret = PTP_Transaction(EOS_OC_MoveFocus, 0, 1, data, 0, NULL);
			steps--;
			wdt_reset();
			if(PTP_Error) resetConnection(); else _delay_ms(50);
		}
		return ret;
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		uint8_t wasInLiveView = modeLiveView;

		if(!ret) blockWhileBusy(1000);
		if(!wasInLiveView) ret = liveView(1);
		_delay_ms(100);

		while(steps > 0 && !ret)
		{
			if(!ret)
			{
        blockWhileBusy(1000);
        if(move > 0) data[0] = 1; else data[0] = 2; 
        if(move > 3) move = 3;
        if(move < -3) move = -3;
        if(move == 1 || move == -1) data[1] = (uint32_t) 0x0A; // small steps
        if(move == 2 || move == -2) data[1] = (uint32_t) 0x28; // medium steps
        if(move == 3 || move == -3) data[1] = (uint32_t) 0x50; // large steps
				ret = PTP_Transaction(NIKON_OC_MoveFocus, 0, 2, data, 0, NULL);
			}
			if(PTP_Error) resetConnection(); else _delay_ms(50);
			wdt_reset();
			steps--;
		}
		
		if(!ret) blockWhileBusy(1000);
		if(!wasInLiveView && !ret) ret = liveView(0);
		return ret;
	}
	return 0;
}

uint8_t PTP::blockWhileBusy(int16_t timeoutMS)
{
	while(timeoutMS > 0)
	{
		wdt_reset();
    _delay_ms(10);
		checkEvent();
		if(!busy) return 0;
    timeoutMS -= 10;
	}
	return 1;
}

uint8_t PTP::checkEvent()
{
	uint8_t ret = 0;
	uint32_t event_size;
	uint32_t event_type;
	uint32_t event_item;
	uint32_t event_value;
	uint32_t i = 0;
	static uint8_t count = 0;
	static uint32_t busySeconds = 0;

	if(ready == 0) return 0;
	if(bulb_open) return 0; // Because the bulb is closed asynchronously (by the clock), this prevents collisions

	if(busy) // auto reset for busy flag
	{
		if(busySeconds)
		{
			if(clock.Seconds() - busySeconds >= BUSY_TIMEOUT_SECONDS)
			{
				busySeconds = 0;
				busy = false;
			}
		}
		else
		{
			busySeconds = clock.Seconds();
		}
	}
	
	if(PTP_protocol == PROTOCOL_NIKON) // NIKON =======================================================
	{
		if(supports.cameraReady)
		{
			PTP_IgnoreErrorsForNextTransaction = true;
			ret = PTP_Transaction(NIKON_OC_CAMERA_READY, 0, 0, NULL, 0, NULL);
			if(PTP_Response_Code != PTP_RESPONSE_OK)//0x2019)
			{
				wdt_reset();
				busy = true;
				return 0;
			}
			busy = false;
		}
		else
		{
			//busy = false;
		}
		if(supports.event)
		{
			ret = PTP_Transaction(NIKON_OC_EVENT_GET, 1, 0, NULL, 0, NULL);
			if(ret) return PTP_RETURN_ERROR;
			uint16_t nevents, tevent;
			memcpy(&nevents, &PTP_Buffer[i], sizeof(uint16_t));
			i += sizeof(uint16_t);
			while(i < PTP_Bytes_Received)
			{
				wdt_reset();
				memcpy(&tevent, &PTP_Buffer[i], sizeof(uint16_t));
				i += sizeof(uint16_t);
				memcpy(&event_value, &PTP_Buffer[i], sizeof(uint32_t));
				i += sizeof(uint32_t);
				switch(tevent)
				{
					case PTP_EC_OBJECT_CREATED:
						currentObject = event_value; // Save the object ID for later retrieving the thumbnail
						//DEBUG(PSTR("\r\n Object added: "));
						//sendHex((char *)&currentObject);
						break;
					case PTP_EC_PROPERTY_CHANGED:
						PTP_need_update = true;
						//DEBUG(PSTR("\r\n Property: "));
						//sendHex((char *)&event_value);
						break;
					default:
						//DEBUG(PSTR("\r\n Event: "));
						//sendHex((char *)&tevent);
						//DEBUG(PSTR("\r\n Param: "));
						//sendHex((char *)&event_value);
						break;
				}
				if(i >= PTP_BUFFER_SIZE) break;
			}
		}
		else
		{
			if(count++ > 10)
			{
				PTP_need_update = true;
				count = 0;
			}
			uint16_t tevent;
			if((tevent = PTP_GetEvent(&event_value)))
			{
				busy = false;
				DEBUG(PSTR("Received Asynchronous Event!\r\n"));
				switch(tevent)
				{
					case PTP_EC_OBJECT_CREATED:
						busy = false;
						currentObject = event_value; // Save the object ID for later retrieving the thumbnail
						//DEBUG(PSTR("\r\n Object added: "));
						//sendHex((char *)&currentObject);
						break;
					case PTP_EC_PROPERTY_CHANGED:
						PTP_need_update = true;
						//DEBUG(PSTR("\r\n Property: "));
						//sendHex((char *)&event_value);
						break;
					default:
						//DEBUG(PSTR("\r\n Event: "));
						//sendHex((char *)&tevent);
						//DEBUG(PSTR("\r\n Param: "));
						//sendHex((char *)&event_value);
						break;
				}
			}
			//PTP_need_update = true;
		}
		if(PTP_need_update) updatePtpParameters();
		return ret;
	}
	if(PTP_protocol != PROTOCOL_EOS) return 0;

	ret = PTP_FIRST_TIME; // CANON ==================================================================
	do {
		if(ret == PTP_FIRST_TIME)
		{
			ret = PTP_Transaction(EOS_OC_EVENT_GET, 1, 0, NULL, 0, NULL);
			i = 0;
			if(ret == PTP_RETURN_ERROR)
			{
				//DEBUG(PSTR("ERROR checking events!\r\n"));
				//DEBUG(PSTR("     PTP_Response_Code: "));
				//DEBUG(PTP_Response_Code);
				//DEBUG_NL();
				//DEBUG(PSTR("     PTP_Error: "));
				//DEBUG(PTP_Error);
				//DEBUG_NL();
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
			//DEBUG(PSTR("Fetching next data packet... \r\n"));
			ret = PTP_FetchData(0);
			if(ret == PTP_RETURN_ERROR)
			{
				//DEBUG(PSTR("Error fetching packet!"));
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
					//DEBUG(PSTR("Pre-fetching next data packet... \r\n"));
					ret = PTP_FetchData(PTP_BUFFER_SIZE - i);
					if(ret == PTP_RETURN_ERROR) return ret;
					i = 0;
					continue;
				}
				else
				{
					//DEBUG(PSTR("Pre-fetch Incomplete! \r\n"));
					return PTP_RETURN_ERROR;
				}
			}
			memcpy(&event_size, &PTP_Buffer[i], sizeof(uint32_t));
			if(event_size == 0)
			{
				//DEBUG(PSTR("ERROR: Zero-length\r\n"));
				return PTP_RETURN_ERROR;
			}
			if((event_size + i) > PTP_BUFFER_SIZE)
			{
				if(ret == PTP_RETURN_DATA_REMAINING)
				{
					if(event_size > PTP_BUFFER_SIZE)
					{
						//DEBUG(PSTR("Too Big: "));
						//DEBUG(event_size);
						//DEBUG_NL();
						//DEBUG(PSTR(" i: "));
						//DEBUG(i);
						//DEBUG_NL();
						//for(uint16_t x = 0; x < 4*5; x++)
						//{
						//	sendByte(PTP_Buffer[i + x - 4]);
						//}
						event_size -= (PTP_BUFFER_SIZE - i);
						while(event_size > PTP_BUFFER_SIZE && ret == PTP_RETURN_DATA_REMAINING)
						{
							ret = PTP_FetchData(0);
						 	event_size -= PTP_Bytes_Received;
							//DEBUG(PSTR(" Received: "));
							//DEBUG(PTP_Bytes_Received);
							//DEBUG_NL();
						}
						i = event_size;
						//DEBUG(PSTR("Checking for non-zero...\r\n"));
						while(ret == PTP_RETURN_DATA_REMAINING)
						{
							uint8_t tbreak = 0;
							for(uint16_t x = i; x < PTP_Bytes_Received; x++)
							{
								if(PTP_Buffer[x] != 0)
								{
									//DEBUG(PSTR("Found non-zero!\r\n"));
									i = x;
									tbreak = 1;
									break;
								}
							}
							if(tbreak) break;
							i = 0;
							ret = PTP_FetchData(0);
							//DEBUG(PSTR(" Received: "));
							//DEBUG(PTP_Bytes_Received);
							//DEBUG_NL();
						}
						//DEBUG(PSTR(" i: "));
						//DEBUG(i);
						//DEBUG_NL();
						//for(uint16_t x = i; x < PTP_Bytes_Received; x++)
						//{
						//	sendByte(PTP_Buffer[x]);
						//}
						continue;
					}
					else
					{
						//DEBUG(PSTR("Fetching next data packet (2)... \r\n"));
						ret = PTP_FetchData(PTP_BUFFER_SIZE - i);
						if(ret == PTP_RETURN_ERROR) return ret;
						i = 0;
						continue;

					}
				}
				else
				{
					//DEBUG(PSTR("Incomplete! \r\n"));
					return PTP_RETURN_ERROR;
				}
			}

			memcpy(&event_type, &PTP_Buffer[i + sizeof(uint32_t) * 1], sizeof(uint32_t));
			memcpy(&event_item, &PTP_Buffer[i + sizeof(uint32_t) * 2], sizeof(uint32_t));
			memcpy(&event_value, &PTP_Buffer[i + sizeof(uint32_t) * 3], sizeof(uint32_t));

			//#ifdef EXTENDED_DEBUG
			//if(i > 1200)
			//{
			//DEBUG(PSTR(" Event: "));
			//DEBUG(i);
			//DEBUG_NL();
			//sendHex((char *) &event_size);
			//DEBUG(PSTR("        "));
			//sendHex((char *) &event_type);
			//DEBUG(PSTR("        "));
			//sendHex((char *) &event_item);
			//DEBUG(PSTR("        "));
			//sendHex((char *) &event_value);
			//DEBUG_NL();
			//}
			//#endif

			if(event_type == EOS_EC_PROPERTY_CHANGE)
			{
				switch(event_item)
				{
					case EOS_DPC_ISO:
						isoPTP = event_value;
						#ifdef EXTENDED_DEBUG
						//DEBUG(PSTR(" ISO:"));
						//DEBUG(event_value);
						//DEBUG_NL();
						#endif
						break;
					case EOS_DPC_SHUTTER:
						shutterPTP = event_value;
						#ifdef EXTENDED_DEBUG
						//DEBUG(PSTR(" SHUTTER:"));
						//DEBUG(event_value);
						//DEBUG_NL();
						#endif
						break;
					case EOS_DPC_APERTURE:
						aperturePTP = event_value;
						#ifdef EXTENDED_DEBUG
						//DEBUG(PSTR(" APERTURE:"));
						//DEBUG(event_value);
						//DEBUG_NL();
						#endif
						break;
					case EOS_DPC_MODE:
						modePTP = event_value;
						#ifdef EXTENDED_DEBUG
						//DEBUG(PSTR(" MODE:"));
						//DEBUG(event_value);
						//DEBUG_NL();
						#endif
						break;
					case EOS_DPC_LiveView:
						//DEBUG(PSTR(" LV:"));
						if(event_value) modeLiveView = true; else modeLiveView = false;
						//#ifdef EXTENDED_DEBUG
						//if(modeLiveView) {DEBUG(PSTR("ON"));} else {DEBUG(PSTR("OFF"));}
						//DEBUG_NL();
						//#endif
						break;
					case EOS_DPC_Video:
						DEBUG(PSTR(" VIDEO:"));
						if(event_value == 4) recording = true; else recording = false;
						//#ifdef EXTENDED_DEBUG
						//if(recording) {DEBUG(PSTR("Recording"));} else {DEBUG(PSTR("OFF"));}
						//DEBUG_NL();
						//#endif
						break;
					case EOS_DPC_PhotosRemaining:
						photosRemaining = event_value;
						//#ifdef EXTENDED_DEBUG
						//DEBUG(PSTR(" Space Available:"));
						//DEBUG(event_value);
						//DEBUG_NL();
						//#endif
						break;
					case EOS_DPC_AFMode:
						autofocus = (event_value != 3);
						//#ifdef EXTENDED_DEBUG
						//DEBUG(PSTR(" AF mode:"));
						//DEBUG(event_value);
						//DEBUG_NL();
						//#endif
						break;
					case EOS_DPC_VideoMode:
						if(event_value == 1) videoMode = true; else videoMode = false;
						//#ifdef EXTENDED_DEBUG
						//DEBUG(PSTR(" Video Mode:"));
						//if(videoMode) {DEBUG(PSTR(" ON"));} else  {DEBUG(PSTR(" OFF"));}
						//DEBUG_NL();
						//#endif
						break;
					default:
						//#ifdef EXTENDED_DEBUG
						//DEBUG(PSTR(" Prop: "));
						//DEBUG(event_item);
						//DEBUG(PSTR(", value: "));
						//DEBUG(event_value);
						//DEBUG_NL();
						//#endif
						break;
				}
			}
			else if(event_type == EOS_EC_PROPERTY_VALUES)
			{
				//#ifdef EXTENDED_DEBUG
				//DEBUG(PSTR(" Value List: "));
				//sendHex((char *)&event_item);
				//DEBUG_NL();
				//#endif

				uint32_t x;
				uint8_t ti;
				switch(event_item)
				{
					case EOS_DPC_ISO:
						ti = 0;
						for(x = 0; x < event_size / sizeof(uint32_t) - 5; x++)
						{
							isoAvail[ti] = PTP::isoEv(PTP_Buffer[i+(x+5)*sizeof(uint32_t)]);
							if(isoAvail[ti] > 0) ti++;
						}
						isoAvailCount = ti;//x;
						supports.iso = isoAvailCount > 0;

						//#ifdef EXTENDED_DEBUG
						//DEBUG(PSTR("\r\n ISO Avail Count: 0x"));
						//DEBUG(isoAvailCount);
						//DEBUG_NL();
						//#endif

						break;
					case EOS_DPC_SHUTTER:
						ti = 0;
						for(x = 0; x < event_size / sizeof(uint32_t) - 5; x++)
						{
							shutterAvail[x] = PTP::shutterEv(PTP_Buffer[i+(x+5)*sizeof(uint32_t)]);
							if(shutterAvail[ti] > 0) ti++;
						}
						shutterAvailCount = ti;//x;
						supports.shutter = shutterAvailCount > 0;
						
						//#ifdef EXTENDED_DEBUG
						//DEBUG(PSTR("\r\n Shutter Avail Count: 0x"));
						//DEBUG(shutterAvailCount);
						//DEBUG_NL();
						//#endif

						break;
					case EOS_DPC_APERTURE:
						ti = 0;
						for(x = 0; x < event_size / sizeof(uint32_t) - 5; x++)
						{
							apertureAvail[ti] = PTP::apertureEv(PTP_Buffer[i+(x+5)*sizeof(uint32_t)]);
							if(apertureAvail[ti] > 0) ti++;
						}
						apertureAvailCount = ti;//x;
						supports.aperture = apertureAvailCount > 0;

						//#ifdef EXTENDED_DEBUG
						//DEBUG(PSTR("\r\n Aperture Avail Count: "));
						//DEBUG(apertureAvailCount);
						//DEBUG_NL();
						//#endif

						break;
					/*case EOS_DPC_MODE: // Always lists 0 since this is supposedly not writable
						for(x = 0; x < event_size / sizeof(uint32_t) - 5; x++)
						{
							apertureAvail[x] = (uint8_t) PTP_Buffer[i+(x+5)*sizeof(uint32_t)];
						}
						modeAvailCount = x;
						//supports.mode = modeAvailCount > 0;

						//#ifdef EXTENDED_DEBUG
						DEBUG(PSTR("\r\n Mode Avail Count: "));
						DEBUG(modeAvailCount);
						DEBUG_NL();
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

				//#ifdef EXTENDED_DEBUG
				//DEBUG(PSTR("\r\n Object added: "));
				//sendHex((char *)&currentObject);
				//#endif

				//getThumb(currentObject);
			}
			else if(event_type == EOS_EC_WillShutdownSoon)
			{
//				DEBUG(PSTR("Keeping camera on\r\n"));
//				PTP_Transaction(EOS_OC_KeepDeviceOn, 0, 0, NULL);
			}
			else if(event_type > 0)
			{
				//DEBUG(PSTR("\r\n Unknown: "));
				//sendHex((char *)&event_type);
				//DEBUG(PSTR("\r\n    Size: "));
				//DEBUG(event_size);
				//DEBUG_NL();
				//DEBUG(PSTR("\r\n  Value1: "));
				//sendHex((char *)&event_value);
				//DEBUG(PSTR("\r\n  Value2: "));
				//sendHex((char *)(&event_value+4));
				//DEBUG(PSTR("\r\n  Value3: "));
				//sendHex((char *)(&event_value+8));
			}
			i += event_size;
			wdt_reset();
		}
	} while(ret == PTP_RETURN_DATA_REMAINING);
	return 0;
}

//void sendHex(char *hex)
//{
//	wdt_reset();
//	for(uint8_t i = 4; i > 0; i--)
//	{
//		char b[4];
//		b[0] = (hex[i-1] >> 4) + '0'; if(b[0] > '9') b[0] += 7;
//		b[1] = (hex[i-1] & 0x0F) + '0'; if(b[1] > '9') b[1] += 7;
//		b[2] = ' ';
//		b[3] = 0;
//		DEBUG(b);
//		if(i == 1)
//		{
//			b[0] = '\r';
//			b[1] = '\n';
//			b[2] = 0;
//			DEBUG(b);
//		}
//	}
//}
//
//void sendByte(char byte)
//{
//	wdt_reset();
//	char b[4];
//	b[0] = (byte >> 4) + '0'; if(b[0] > '9') b[0] += 7;
//	b[1] = (byte & 0x0F) + '0'; if(b[1] > '9') b[1] += 7;
//	b[2] = ' ';
//	b[3] = 0;
//	DEBUG(b);
//	b[0] = '\r';
//	b[1] = '\n';
//	b[2] = 0;
//	DEBUG(b);
//}

uint8_t PTP::close()
{
	DEBUG(PSTR("PTP Closed\r\n"));
	static_ready = 0;
	ready = 0;
	busy = false;
	bulb_open = false;
	supports = (CameraSupports_t)
	{
		.capture = false,
		.bulb = false,
		.iso = false,
		.shutter = false,
		.aperture = false,
		.focus = false,
		.video = false,
		.cameraReady = false,
		.event = false
	};
	isoPTP = 0xFF;
	aperturePTP = 0xFF;
	shutterPTP = 0xFF;
	isoAvailCount = 0;
	apertureAvailCount = 0;
	shutterAvailCount = 0;
	return 0;
}

void PTP::resetConnection()
{
	wdt_reset();

	USB_ResetInterface();
	close();

	//DEBUG(PSTR("Disabling USB\r\n"));
	PTP_Disable();
	hardware_USB_SetDeviceMode();
	hardware_USB_Disable();
	
	#ifdef USB_ENABLE_PIN
	    setOut(USB_ENABLE_PIN);
	    setHigh(USB_ENABLE_PIN);
	#endif
	
	wdt_reset();
	_delay_ms(100);

	//DEBUG(PSTR("Enabling USB\r\n"));
	
	hardware_USB_Enable();
	#ifdef USB_ENABLE_PIN
	    setOut(USB_ENABLE_PIN);
	    setLow(USB_ENABLE_PIN);
	#endif

	hardware_USB_SetHostMode();
	PTP_Enable();
}

uint8_t PTP::capture()
{
	if(!static_ready) return 0;
	if(isInBulbMode()) manualMode();
	busy = true;
	if(PTP_protocol == PROTOCOL_EOS)
	{
		if(PTP_Transaction(EOS_OC_CAPTURE, 0, 0, NULL, 0, NULL)) return PTP_RETURN_ERROR;
	}
	else if(PTP_protocol == PROTOCOL_NIKON && supports_nikon_capture)
	{
		data[0] = 0xffffffff; // no af
		data[1] = 0x00000000; // capture to card
		//if(PTP_Transaction(NIKON_OC_CAPTURE2, 0, 2, data, 0, NULL)) return PTP_RETURN_ERROR;
		if(PTP_Transaction(NIKON_OC_CAPTURE, 0, 0, NULL, 0, NULL)) return PTP_RETURN_ERROR;
	}
	else
	{
		data[0] = 0x00000000;
		data[1] = 0x00000000;
		if(PTP_Transaction(PTP_OC_CAPTURE, 0, 2, data, 0, NULL)) return PTP_RETURN_ERROR;;
	}
	if(PTP_Response_Code != PTP_RESPONSE_OK) return 1;
	return 0;
}

uint8_t PTP::videoStart()
{
	if(!static_ready) return 0;
	if(!supports.video) return PTP_RETURN_ERROR;
	if(PTP_protocol == PROTOCOL_EOS && modeLiveView)
	{
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
		recording  = false;
		setEosParameter(EOS_DPC_Video, 0x00);
	}
	return 0;
}

uint8_t PTP::isInBulbMode()
{
	if(modePTP == 0x04 || shutter() == BULB_EV_CODE) return 1; else return 0; 
}

uint8_t PTP::bulbMode()
{
	if(conf.camera.modeSwitch == USB_CHANGE_MODE_DISABLED) return 0;
	if(isInBulbMode()) return 0;

	if(PTP_protocol == PROTOCOL_EOS)
	{
		bool bulbAsShutter = false;
		for(uint8_t i = 0; i < shutterAvailCount; i++)
		{
			if(shutterAvail[i] == BULB_EV_CODE)
			{
				bulbAsShutter = true;
				break;
			}
		}
		if(bulbAsShutter)
		{
			preBulbShutter = shutter();
			setShutter(BULB_EV_CODE);
		}
		else
		{
			preBulbShutter = 0;
			return setEosParameter(EOS_DPC_MODE, 0x04); // Bulb Mode
		}
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		preBulbShutter = shutter();
		setPtpParameter(0xd100, (uint32_t)0xffffffff);
	}
	if(PTP_Response_Code != PTP_RESPONSE_OK) return 1;
	return 0;
}

uint8_t PTP::manualMode()
{
	if(conf.camera.modeSwitch == USB_CHANGE_MODE_DISABLED) return 0;
	if(!isInBulbMode()) return 0;

	if(PTP_protocol == PROTOCOL_EOS)
	{
		if(preBulbShutter)
		{
			uint8_t tmp = preBulbShutter;
			preBulbShutter = 0;
			setShutter(tmp);
		}
		else
		{
			if(shutter() == BULB_EV_CODE)
			{
				return setShutter(69); // 1/400
			}
			else
			{
				return setEosParameter(EOS_DPC_MODE, 0x03); // Manual Mode
			}
		}
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		//DEBUG(PSTR("ManualMode: "));
		//DEBUG(preBulbShutter);
		//DEBUG_NL();
		if(preBulbShutter)
		{
			uint8_t tmp = preBulbShutter;
			preBulbShutter = 0;
			setShutter(tmp);
		}
		else
		{
			setShutter(69); // 1/400
		}
	}
	if(PTP_Response_Code != PTP_RESPONSE_OK) return 1;
	return 0;
}

uint8_t PTP::bulbStart()
{
	if(!static_ready) return PTP_RETURN_ERROR;
	//if(bulb_open) return 1;
	if(!supports.bulb) return PTP_RETURN_ERROR;
	bulb_open = true;
	busy = true;
	bulbMode();
	if(PTP_protocol == PROTOCOL_EOS)
	{
		data[0] = 0x03;
		data[1] = 0x00;
		if(ptpBulbMode)
		{
			if(PTP_Transaction(EOS_OC_SETUILOCK, 0, 0, NULL, 0, NULL)) return 1; // SetUILock
			if(PTP_Transaction(EOS_OC_BULBSTART, 0, 0, NULL, 0, NULL)) return 1; // Bulb Start
		}
		else
		{
			if(PTP_Transaction(EOS_OC_REMOTE_RELEASE_ON, 0, 2, data, 0, NULL)) return PTP_RETURN_ERROR; // Bulb Start
		}
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		data[0] = 0xFFFFFFFF;
		data[1] = 0x00000001;
		if(PTP_Transaction(NIKON_OC_BULBSTART, 0, 2, data, 0, NULL)) return PTP_RETURN_ERROR; // Bulb Start
	}
	//DEBUG(PSTR("Res: "));
	//DEBUG(PTP_Response_Code);
	//DEBUG_NL();
	if(PTP_Response_Code != PTP_RESPONSE_OK) return PTP_RETURN_ERROR;
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
		if(ptpBulbMode)
		{
			if(PTP_Transaction(EOS_OC_BULBEND, 0, 0, NULL, 0, NULL)) return 1; // Bulb End
			if(PTP_Transaction(EOS_OC_RESETUILOCK, 0, 0, NULL, 0, NULL)) return 1; // ResetUILock
		}
		else
		{
			if(PTP_Transaction(EOS_OC_REMOTE_RELEASE_OFF, 0, 1, data, 0, NULL)) return PTP_RETURN_ERROR; // Bulb End
		}
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		data[0] = 0x0000D800;
		data[1] = 0x00000001; // This parameter varies (0x01, 0x21) -- I don't know what it means
		if(PTP_Transaction(NIKON_OC_BULBEND, 0, 2, data, 0, NULL)) return PTP_RETURN_ERROR; // Bulb End
	}
	bulb_open = false;
	if(PTP_Response_Code != PTP_RESPONSE_OK) return PTP_RETURN_ERROR;
	return 0;
}

uint8_t PTP::setISO(uint8_t ev)
{
	if(ev == 0xff) return 1;
	isoPTP = isoEvPTP(ev);
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
	//manualMode();
	shutterPTP = shutterEvPTP(ev);
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
	aperturePTP = apertureEvPTP(ev);
	if(PTP_protocol == PROTOCOL_EOS)
	{
		return setEosParameter(EOS_DPC_APERTURE, apertureEvPTP(ev));
	}
	else
	{
		return setPtpParameter(NIKON_DPC_APERTURE, (uint16_t)apertureEvPTP(ev));
	}
}

uint8_t PTP::setFocus(uint8_t af)
{
	if(PTP_protocol == PROTOCOL_EOS)
	{
		return setEosParameter(EOS_DPC_AFMode, (af ? 0x0 : 0x03));
	}
	else if(PTP_protocol == PROTOCOL_NIKON)
	{
		//return setPtpParameter(NIKON_DPC_AutofocusMode, (uint16_t)(af ? 32784 : 0));
		return setPtpParameter(NIKON_DPC_AutofocusMode, (uint8_t)(af ? 0x0 : 0x04));
	}
	return 0;
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
	if(!ret && PTP_Bytes_Received > 10)
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
	if(!ret && PTP_Bytes_Received > 10)
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
	if(!ret && PTP_Bytes_Received == sizeof(uint16_t)) *value = (uint16_t)PTP_Buffer;
	return ret;
}

uint8_t PTP::updatePtpParameters(void)
{
	PTP_need_update = 0;
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
		if(PTP_Transaction(PTP_OC_PROPERTY_LIST, 1, 1, data, 0, NULL)) return PTP_RETURN_ERROR;
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
		if(PTP_Transaction(PTP_OC_PROPERTY_LIST, 1, 1, data, 0, NULL)) return PTP_RETURN_ERROR;
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
		if(PTP_Transaction(PTP_OC_PROPERTY_LIST, 1, 1, data, 0, NULL)) return PTP_RETURN_ERROR;
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

		//if(supports.AFMode)
		//{
		//	uint16_t tmp;
		//	getPtpParameter(NIKON_DPC_AutofocusMode, &tmp);
		//	autofocus = (tmp != 0x04);
		//}
	}
	return 0;
}

	// This method still needs work... //
uint8_t PTP::getPropertyInfo(uint16_t prop_code, uint8_t expected_size, uint16_t *count, uint8_t *current, uint8_t *list)
{

	//First check that the prop_code is in the list of supported properties (not implemented)

	// Send a command to the device to describe this property.
	data[0] = (uint32_t)prop_code;
	if(PTP_Transaction(PTP_OC_PROPERTY_LIST, 1, 1, data, 0, NULL)) return PTP_RETURN_ERROR;

	// data[0]
	// data[1] -- Property code back
	uint16_t prop;
	memcpy(&prop, &PTP_Buffer[0], sizeof(uint16_t));
    if(prop != prop_code) return PTP_RETURN_ERROR;

	//DEBUG(PSTR("PROP="));
	//DEBUG(prop);
	//DEBUG_NL();

	// data[2]
	// data[3] -- data type code
	// Setting the type code for the property also turns its
	// support flag on.
    uint16_t type = 0;
	memcpy(&type, &PTP_Buffer[2], sizeof(uint16_t));

	//DEBUG(PSTR("TYPE="));
	//DEBUG(type);
	//DEBUG_NL();

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

	//DEBUG(PSTR("CURRENT="));
	//DEBUG(*((uint16_t*)current));
	//DEBUG_NL();

	// The form flag...
    uint8_t form = PTP_Buffer[index];
    index++;

	//DEBUG(PSTR("FORM="));
	//DEBUG(form);
	//DEBUG_NL();

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

		//DEBUG(PSTR("COUNT="));
		//DEBUG(*count);
		//DEBUG_NL();


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
		//DEBUG(PSTR("Error Retrieving thumbnail (c)!\r\n"));
		return 0;
	}
	return ret;
}
uint8_t PTP::getThumb(uint32_t handle)
{
	data[0] = handle;
	uint8_t ret = PTP_Transaction(PTP_OC_GET_THUMB, RECEIVE_DATA, 1, data, 0, NULL);
	if(ret == PTP_RETURN_ERROR)
	{
		//DEBUG(PSTR("Error Retrieving thumbnail!\r\n"));
		return 0;
	}
	else
	{
		//DEBUG(PSTR("Obj Size: "));
		//DEBUG(PTP_Bytes_Total);
		//DEBUG_NL();
		return ret;
	}
}

uint8_t PTP::writeFile(char *name, uint8_t *data, uint16_t dataSize)
{
    ptp_object_info objectinfo;
    memset(&objectinfo,0,sizeof(objectinfo));

    for(uint8_t i = 0; i < sizeof(objectinfo.filename); i += 2)
    {
    	objectinfo.filename[i] = name[i / 2];
    	if(name[i / 2] == 0) break;
    }

    objectinfo.storage_id = 0x0;
    objectinfo.object_format = PTP_OFC_Text;
	sendObjectInfo(0x0, 0x0, &objectinfo);
	sendObject(data, dataSize);
	return 0;
}

uint8_t PTP::sendObjectInfo(uint32_t storage, uint32_t parent, ptp_object_info *objectinfo)
{
	void *payload;
	payload = objectinfo;
	data[0] = storage;
	data[1] = parent;
	return PTP_Transaction(PTP_OC_SendObjectInfo, NO_RECEIVE_DATA, 2, data, sizeof(ptp_object_info), (uint8_t *)payload);
}

uint8_t PTP::sendObject(uint8_t *data, uint16_t dataSize)
{
	return PTP_Transaction(PTP_OC_SendObject, NO_RECEIVE_DATA, 0, NULL, dataSize, data);
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

