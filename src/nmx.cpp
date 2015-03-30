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
extern settings_t conf;
extern Clock clock;

uint8_t connectedResponse = 0;
uint32_t lastCommandMs = 0;

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
	currentPos = 0;
	connectedResponse = 0;
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
	return move(dir, steps, 1);
}

uint8_t NMX::move(uint8_t dir, uint32_t steps, uint8_t update)
{
  while(running())
  {
    wdt_reset();
  }

  uint8_t buf[5], buf2[4];
  memcpy(buf2, &steps, sizeof(uint32_t));
  buf[1] = buf2[3];
  buf[2] = buf2[2];
  buf[3] = buf2[1];
  buf[4] = buf2[0];
  buf[0] = dir;

  if(update)
  {
    if(dir == 0) currentPos += steps; else currentPos -= steps;
  }

  return sendCommand(0xF, 5, buf);  
}

uint8_t NMX::stop()
{
  return sendCommand(0x4, 0, 0);  
}


uint8_t NMX::enable()
{
	if(!enabled)
	{
		enabled = true;
		uint8_t buf = 1;
		sendCommand(0x3, 1, &buf); //enable

		if(motorAddress == 1) buf = conf.motionPowerSave1;
		if(motorAddress == 2) buf = conf.motionPowerSave2;
		if(motorAddress == 3) buf = conf.motionPowerSave3;
		if(buf) buf = 1;
		sendCommand(0x2, 1, &buf); //disable sleep (hold between moves)

    if(motorAddress == 1 && conf.motionMicroSteps1) sendCommand(0x6, 1, &conf.motionMicroSteps1);
    if(motorAddress == 2 && conf.motionMicroSteps2) sendCommand(0x6, 1, &conf.motionMicroSteps2);
    if(motorAddress == 3 && conf.motionMicroSteps3) sendCommand(0x6, 1, &conf.motionMicroSteps3);

    uint8_t backlash[2], buf2[2];		
    float speed = 0.0;
    if(motorAddress == 1 && conf.motionSpeed1) speed = (float)conf.motionSpeed1 * (float)conf.motionMicroSteps1;
    if(motorAddress == 2 && conf.motionSpeed2) speed = (float)conf.motionSpeed2 * (float)conf.motionMicroSteps2;
    if(motorAddress == 3 && conf.motionSpeed3) speed = (float)conf.motionSpeed3 * (float)conf.motionMicroSteps3;
    if(speed > 0.0)
    {
      buf2[0] = 255;
      buf2[1] = 255;
      sendCommand(0x7, 2, buf2); // no max speed limit
      setSpeed(speed);
      setAccel(speed * 2);
    }
    uint16_t steps = 0;
    if(motorAddress == 1) steps = conf.motionBacklash1;
    if(motorAddress == 2) steps = conf.motionBacklash2;
    if(motorAddress == 3) steps = conf.motionBacklash3;
    memcpy(backlash, &steps, sizeof(uint16_t));
		buf2[0] = backlash[1];
		buf2[1] = backlash[0];
		return sendCommand(0x5, 2, buf2); //set backlash
	}
	return 0;
}

uint8_t NMX::disable()
{
	enabled = false;
	uint8_t buf = 1;
	sendCommand(0x2, 1, &buf);	
  buf = 0;
	return sendCommand(0x3, 1, &buf);	
}

uint8_t NMX::running()
{
	if(sendQuery(0x6B) != 0) return 1; else return 0;
}

uint8_t NMX::checkConnected()
{
	if(!remote.nmx)
	{
		connected = 0;
		connectedResponse = 0;
	}
	else
	{
		if(connectedResponse == 0)
		{
			connectedResponse = (uint8_t)sendQueryGeneral(3, 0, 0x7C, 10) | 0b10000000;
		}
		if(connectedResponse & (1 << (motorAddress - 1)))
		{
			connected = 1;
		}
		else
		{
			connected = 0;
		}
	}
	return connected;
}

uint8_t NMX::setSpeed(float rate)
{
  uint8_t *ptr = (uint8_t*)&rate;
  uint8_t buf[4];
  buf[3] = ptr[0];
  buf[2] = ptr[1];
  buf[1] = ptr[2];
  buf[0] = ptr[3];
  return sendCommand(0xD, 4, buf);
}

uint8_t NMX::setAccel(float rate)
{
  uint8_t *ptr = (uint8_t*)&rate;
  uint8_t buf[4];
  buf[3] = ptr[0];
  buf[2] = ptr[1];
  buf[1] = ptr[2];
  buf[0] = ptr[3];
  return sendCommand(0xE, 4, buf);
}

// returns response as uint32 (max 4)
uint32_t NMX::sendQueryGeneral(uint8_t node, uint8_t motor, uint8_t command, uint8_t delay)
{
	uint8_t b;
	uint32_t value = 0;
	if(sendCommandGeneral(node, motor, command, 0, 0))
	{
		while(delay)
		{
			_delay_ms(100);
			delay--;
		}
		bt.sendCMD(PSTR("ATGR,0,24\r"));
		if(bt.checkOK())
		{
			char *buf;
			if(bt.waitEvent(STR("GATT_VAL"), &buf))
			{
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

// returns response as uint32 (max 4)
uint32_t NMX::sendQuery(uint8_t command)
{
	return sendQueryGeneral(nodeAddress, motorAddress, command, 1);
}

uint8_t NMX::sendCommandGeneral(uint8_t node, uint8_t motor, uint8_t command, uint8_t dataLength, uint8_t *data)
{
	if(!remote.nmx) return 0;

	if(clock.Ms() < lastCommandMs) lastCommandMs = clock.Ms();
	while(clock.Ms() - lastCommandMs < NMX_COMMAND_SPACING_MS);

	lastCommandMs = clock.Ms();

	char hexbuf[3];
	bt.sendCMD(PSTR("ATGW,0,28,0,0000000000FF"));

	uint8tohex(hexbuf, node);
	bt.sendCMD((char*)hexbuf);

	uint8tohex(hexbuf, motor);
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

uint8_t NMX::sendCommand(uint8_t command, uint8_t dataLength, uint8_t *data)
{
	return sendCommandGeneral(nodeAddress, motorAddress, command, dataLength, data);
}

