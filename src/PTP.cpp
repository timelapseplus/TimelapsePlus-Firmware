#include <avr/pgmspace.h>
#include "PTP_Codes.h"
#include "PTP_Driver.h"
#include "PTP.h"
#include "bluetooth.h"
#include "tldefs.h"

const propertyDescription_t EOS_Aperture_List[] PROGMEM = {
    { "1.2", 0x0c },
    { "1.4", 0x10 },
    { "1.6", 0x13 },
    { "1.8", 0x14 },
    { "2.0", 0x18 },
    { "2.2", 0x1b },
    { "2.5", 0x1c },
    { "2.8", 0x20 },
    { "3.2", 0x23 },
    { "3.5", 0x24 },
    { "4.0", 0x28 },
    { "4.5", 0x2b },
    { "5.0", 0x2d },
    { "5.6", 0x30 },
    { "6.3", 0x33 },
    { "7.1", 0x35 },
    { "8.0", 0x38 },
    { "9.0", 0x3b },
    { "10.0", 0x3d },
    { "11.0", 0x40 },
    { "13.0", 0x43 },
    { "14.0", 0x45 },
    { "16.0", 0x48 },
    { "18.0", 0x4b },
    { "20.0", 0x4d },
    { "22.0", 0x50 }
};

const propertyDescription_t EOS_Shutter_List[] PROGMEM = {
    {"1/8000", 0xa0 },
    {"1/6400", 0x9d },
    {"1/5000", 0x9b },
    {"1/4000", 0x98 },
    {"1/3200", 0x95 },
    {"1/2500", 0x93 },
    {"1/2000", 0x90 },
    {"1/1600", 0x8d },
    {"1/1250", 0x8b },
    {"1/1000", 0x88 },
    {"1/800", 0x85 },
    {"1/640", 0x83 },
    {"1/500", 0x80 },
    {"1/400", 0x7d },
    {"1/320", 0x7b },
    {"1/250", 0x78 },
    {"1/200", 0x75 },
    {"1/160", 0x73 },
    {"1/125", 0x70 },
    {"1/100", 0x6d },
    {"1/80", 0x6b },
    {"1/60", 0x68 },
    {"1/50", 0x65 },
    {"1/40", 0x63 },
    {"1/30", 0x60 },
    {"1/25", 0x5d },
    {"1/20", 0x5b },
    {"1/15", 0x58 },
    {"1/13", 0x55 },
    {"1/10", 0x53 },
    {"1/8", 0x50 },
    {"1/6", 0x4c },
    {"1/5", 0x4b },
    {"1/4", 0x48 },
    {"1/3", 0x44 },
    {"1/2.5", 0x43 },
    {"1/2", 0x40 },
    {"1/1.6", 0x3d },
    {"1/1.3", 0x3b },
    {"1s", 0x38 },
    {"1.3s", 0x35 },
    {"1.6s", 0x33 },
    {"2s", 0x30 },
    {"2.5s", 0x2d },
    {"3.2s", 0x2b },
    {"4s", 0x28 },
    {"5s", 0x25 },
    {"6s", 0x23 },
    {"8s", 0x20 },
    {"10s", 0x1c },
    {"13", 0x1b },
    {"15s", 0x18 },
    {"20s", 0x14 },
    {"25s", 0x13 },
    {"30s", 0x10 },
    {"bulb", 0x0c }
};

const propertyDescription_t EOS_ISO_List[] PROGMEM = {
    {"100", 0x48 },
    {"125", 0x4b },
    {"160", 0x4d },
    {"200", 0x50 },
    {"250", 0x53 },
    {"320", 0x55 },
    {"400", 0x58 },
    {"500", 0x5b },
    {"640", 0x5d },
    {"800", 0x60 },
    {"1000", 0x63 },
    {"1250", 0x65 },
    {"1600", 0x68 },
    {"2000", 0x6B },
    {"2500", 0x6D },
    {"3200",  0x70 },
    {"4000",  0x73 },
    {"5000",  0x75 },
    {"6400",  0x78 },
    {"6400",  0x78 },
    {"12800",  0x80 }
};

void sendHex(char *hex);

extern BT bt;

PTP::PTP(void)
{
	ready = 0;
}

uint8_t PTP::isoUp(uint16_t id)
{
	uint8_t max = sizeof(EOS_ISO_List) / sizeof(EOS_ISO_List[0]);
	for(uint8_t i = 0; i < max; i++)
	{
		if(pgm_read_word(&EOS_ISO_List[i].id) == id)
		{
			if(i < max - 1)
			{
				setISO(pgm_read_word(&EOS_ISO_List[i + 1].id));
				return 1;
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}

uint8_t PTP::isoDown(uint16_t id)
{
	uint8_t max = sizeof(EOS_ISO_List) / sizeof(EOS_ISO_List[0]);
	for(uint8_t i = 0; i < max; i++)
	{
		if(pgm_read_word(&EOS_ISO_List[i].id) == id)
		{
			if(i > 0)
			{
				setISO(pgm_read_word(&EOS_ISO_List[i - 1].id));
				return 1;
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}

uint8_t PTP::isoName(char name[7], uint16_t id)
{
	for(uint8_t i = 0; i < sizeof(EOS_ISO_List) / sizeof(EOS_ISO_List[0]); i++)
	{
		if(pgm_read_word(&EOS_ISO_List[i].id) == id)
		{
			for(uint8_t b = 0; b < 7; b++) name[b] = pgm_read_word(&EOS_ISO_List[i].name[b]);
			return 1;
		}
	}
	return 0;
}

uint8_t PTP::apertureName(char name[7], uint16_t id)
{
	for(uint8_t i = 0; i < sizeof(EOS_Aperture_List) / sizeof(EOS_Aperture_List[0]); i++)
	{
		if(pgm_read_word(&EOS_Aperture_List[i].id) == id)
		{
			for(uint8_t b = 0; b < 7; b++) name[b] = pgm_read_word(&EOS_Aperture_List[i].name[b]);
			return 1;
		}
	}
	return 0;
}

uint8_t PTP::shutterName(char name[7], uint16_t id)
{
	for(uint8_t i = 0; i < sizeof(EOS_Shutter_List) / sizeof(EOS_Shutter_List[0]); i++)
	{
		if(pgm_read_word(&EOS_Shutter_List[i].id) == id)
		{
			for(uint8_t b = 0; b < 7; b++) name[b] = pgm_read_word(&EOS_Shutter_List[i].name[b]);
			return 1;
		}
	}
	return 0;
}

uint8_t PTP::init()
{
	data[0] = 0x00000001;
	if(PTP_Transaction(EOS_OC_PC_CONNECT, 0, 1, data)) return 1; // PC Connect Mode //
	data[0] = 0x00000001;
	if(PTP_Transaction(EOS_OC_EXTENDED_EVENT_INFO_SET, 0, 1, data)) return 1; // Extended Event Info Mode //

	ready = 1;
	checkEvent();

	supports = (CameraSupports_t) // Should eventually query camera for this data //
	{
		.capture = true,
		.bulb = false,
		.iso = true,
		.shutter = true,
		.aperture = true
	};
	ready = 1;
	return 0;
}

uint8_t PTP::checkEvent()
{
	if(ready == 0) return 0;
	uint8_t ret;
	do {
		ret = PTP_Transaction(EOS_OC_EVENT_GET, 2, 0, NULL);
		if(ret == PTP_RETURN_ERROR)
		{
			bt.send(STR("ERROR!\r\n"));
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
				bt.send(STR("ERROR: Zero-length\r\n"));
				return PTP_RETURN_ERROR;
			}
			if((event_size + i) > PTP_BUFFER_SIZE)
			{
				if(ret == PTP_RETURN_DATA_REMAINING)
				{
					if(event_size > PTP_BUFFER_SIZE)
					{
						bt.send(STR("Too Big: "));
						sendHex((char *) &event_size);
						bt.send(STR(" i: "));
						sendHex((char *) &i);

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
			if(event_type == 0xC189)
			{
				switch(event_item)
				{
					case EOS_DPC_ISO:
						iso = event_value;
						bt.send(STR(" ISO:"));
						sendHex((char *) &event_value);
						break;
					case EOS_DPC_SHUTTER:
						shutter = event_value;
						bt.send(STR(" SHUTTER:"));
						sendHex((char *) &event_value);
						break;
					case EOS_DPC_APERTURE:
						aperture = event_value;
						bt.send(STR(" APERTURE:"));
						sendHex((char *) &event_value);
						break;
				}

				//bt.send(STR("*"));
			}
			else if(event_type == 0xC18A)
			{
				switch(event_item)
				{
					case EOS_DPC_ISO:
					case EOS_DPC_SHUTTER:
					case EOS_DPC_APERTURE:
						bt.send(STR(" Type:"));
						sendHex((char *) &event_type);
						bt.send(STR(" Size:"));
						sendHex((char *) &event_size);
					break;
				}
			}
			i += event_size;
		}
	} while(ret == PTP_RETURN_DATA_REMAINING);
	return 0;
}

void sendHex(char *hex)
{
	for(uint8_t i = 4; i > 0; i--)
	{
		char b[4];
		b[0] = (hex[i-1] >> 4) + '0'; if(b[0] > '9') b[0] += 7;
		b[1] = (hex[i-1] & 0x0F) + '0'; if(b[1] > '9') b[1] += 7;
		b[2] = ' ';
		b[3] = 0;
		bt.send(b);
		if(i == 1)
		{
			b[0] = '\r';
			b[1] = '\n';
			b[2] = 0;
			bt.send(b);
		}
		_delay_ms(10);
	}
}

uint8_t PTP::close()
{
	ready = 0;
	return 0;
	supports = (CameraSupports_t)
	{
		.capture = false,
		.bulb = false,
		.iso = false,
		.shutter = false,
		.aperture = false
	};
}

uint8_t PTP::capture()
{
	return PTP_Transaction(EOS_OC_CAPTURE, 0, 0, NULL);
}

uint8_t PTP::bulbStart()
{
	data[0] = 0xfffffff8;
	data[1] = 0x00001000;
	data[2] = 0x00000000;

	if(PTP_Transaction(0x911A, 0, 3, data)) return 1; // PCHDDCapacity
	if(PTP_Transaction(0x911B, 0, 0, NULL)) return 1; // SetUILock
	if(PTP_Transaction(0x9125, 0, 0, NULL)) return 1; // Bulb Start
	return 0;
}

uint8_t PTP::bulbEnd()
{
    data[0] = 0xffffffff;
	data[1] = 0x00001000;
	data[2] = 0x00000000;
	if(PTP_Transaction(0x911A, 0, 3, data)) return 1; // PCHDDCapacity
    
    data[0] = 0xfffffffc;
	if(PTP_Transaction(0x911A, 0, 3, data)) return 1; // PCHDDCapacity
	if(PTP_Transaction(0x9126, 0, 0, NULL)) return 1; // Bulb End
	if(PTP_Transaction(0x911C, 0, 0, NULL)) return 1; // ResetUILock
	return 0;
}

uint8_t PTP::setISO(uint16_t value)
{
	return setParameter(EOS_DPC_ISO, value);
}

uint8_t PTP::setShutter(uint16_t value)
{
	return setParameter(EOS_DPC_SHUTTER, value);
}

uint8_t PTP::setAperture(uint16_t value)
{
	return setParameter(EOS_DPC_APERTURE, value);
}

uint8_t PTP::setParameter(uint16_t param, uint16_t value)
{
	data[0] = 0x0000000C;
	data[1] = (uint32_t) param;
	data[2] = (uint32_t) value;
	return PTP_Transaction(EOS_OC_PROPERTY_SET, 1, 3, data);
}


