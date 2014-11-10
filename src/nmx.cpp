#include <avr/pgmspace.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <string.h>
#include "tldefs.h"
#include "clock.h"
#include "button.h"
#include "debug.h"
#include "bluetooth.h"
#include "settings.h"
#include "shutter.h"
#include "remote.h"
#include "nmx.h"

extern BT bt;
extern Remote remote;

void uint8tohex(char str[3], uint8_t b)
{
	str[1] = (b % 16);
	if(str[1] < 10) str[1] += '0'; else str[1] += 'A' - 10;
	b -= (b % 16);
	b /= 16;
	str[0] = b;
	if(str[0] < 10) str[0] += '0'; else str[0] += 'A' - 10;
	str[2] = '\0';
}

uint8_t hextouint8(uint8_t *b, char *buf)
{
	*b = 0;
	if(buf[0] >= '0' && buf[0] <= '9')
	{
		*b = (buf[0] - '0') * 16;
	}
	else if(buf[0] >= 'A' && buf[0] <= 'F')
	{
		*b = (buf[0] - 'A' + 10) * 16;
	}
	else
	{
		return 0;
	}
	if(buf[1] >= '0' && buf[1] <= '9')
	{
		*b += (buf[1] - '0');
	}
	else if(buf[1] >= 'A' && buf[1] <= 'F')
	{
		*b += (buf[1] - 'A' + 10);
	}
	else
	{
		return 0;
	}
	return 1;
}

NMX::NMX(uint8_t node, uint8_t motor)
{
	nodeAddress = node;
	motorAddress = motor;
	endPos = 0;
	currentPos = 0;
	stepSize = 256;
}

void NMX::setStart()
{
	endPos += currentPos;
	currentPos = 0;
}

void NMX::setEnd()
{
	endPos = currentPos;
}

uint8_t NMX::moveForward()
{
	return moveSteps(0, stepSize);
}

uint8_t NMX::moveBackward()
{
	return moveSteps(1, stepSize);
}

uint8_t NMX::gotoStart()
{
	return moveToPosition(0);
}

uint8_t NMX::gotoEnd()
{
	return moveToPosition(endPos);
}

uint8_t NMX::moveToPosition(int32_t pos)
{
	if(currentPos > pos)
	{
		return moveSteps(1, (uint32_t)(currentPos - pos));
	}
	else if(currentPos < pos)
	{
		return moveSteps(0, (uint32_t)(pos - currentPos));
	}
	else
	{
		return 0;
	}
}

uint8_t NMX::moveSteps(uint8_t dir, uint32_t steps)
{
	uint8_t buf[5], buf2[4];
	memcpy(buf2, &steps, sizeof(uint32_t));
	buf[1] = buf2[3];
	buf[2] = buf2[2];
	buf[3] = buf2[1];
	buf[4] = buf2[0];
	buf[0] = dir;

	while(running())
	{
		wdt_reset();
	}

	if(dir == 0) currentPos += steps; else currentPos -= steps;

	return sendCommand(0xF, 5, buf);	
}

uint8_t NMX::enable()
{
	uint8_t buf = 1;
	return sendCommand(0x3, 1, &buf);	
	return sendCommand(0x2, 0, &buf);	
}

uint8_t NMX::disable()
{
	uint8_t buf = 0;
	return sendCommand(0x2, 1, &buf);	
	return sendCommand(0x3, 1, &buf);	
}

uint8_t NMX::running()
{
	if(sendQuery(0x6B) == 1) return 1; else return 0;
}

// returns data length in bytes (max 4)
uint32_t NMX::sendQuery(uint8_t command)
{
	uint8_t b;
	uint32_t value = 0;
	if(sendCommand(command, 0, 0))
	{
		bt.sendCMD(PSTR("ATGR,0,24\r"));
		if(bt.checkOK())
		{
			char *buf;
			if(bt.waitEvent(STR("GATT_VAL"), &buf))
			{
				DEBUG(STR("GATT RESPONSE: "));
				DEBUG(buf);

				if(strncmp(buf, STR("GATT_VAL"), 8) == 0 && strlen(buf) >= 21)
				{
					uint8_t len;
					if(hextouint8(&len, &buf[22]))
					{
						if(len <= 4)
						{
							for(uint8_t i = 0; i < len; i++)
							{
								hextouint8(&b, &buf[24 + i * 2]);
								value += ((uint32_t)b << (8 * (len - i - 1)));
							}
							return value;
						}
					}
				}

				return 0;
			}
		}
	}
	return 0;
}


uint8_t NMX::sendCommand(uint8_t command, uint8_t dataLength, uint8_t *data)
{
	if(!remote.nmx) return 0;

	char hexbuf[3];
	bt.sendCMD(PSTR("ATGW,0,28,0,0000000000FF"));

	uint8tohex(hexbuf, nodeAddress);
	bt.sendCMD((char*)hexbuf);

	uint8tohex(hexbuf, motorAddress);
	bt.sendCMD((char*)hexbuf);

	uint8tohex(hexbuf, command);
	bt.sendCMD((char*)hexbuf);

	uint8tohex(hexbuf, dataLength);
	bt.sendCMD((char*)hexbuf);

	for(uint8_t i = 0; i < dataLength; i++)
	{
		uint8tohex(hexbuf, data[i]);
		bt.sendCMD((char*)hexbuf);
	}

	bt.sendCMD(PSTR("\r"));
	return bt.checkOK();
}

