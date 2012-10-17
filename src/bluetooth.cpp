/*
 *  bluetooth.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

#include <LUFA/Drivers/Peripheral/Serial.h>
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include "tldefs.h"
#include "hardware.h"
#include "bluetooth.h"
#include "debug.h"

/******************************************************************
 *
 *   BT::BT
 *
 *
 ******************************************************************/

BT::BT(void)
{
	// Configure Bluetooth //
	BT_INIT_IO;
	Serial_Init(115200, true);
}

/******************************************************************
 *
 *   BT::init
 *
 *
 ******************************************************************/

uint8_t BT::init(void)
{
	_delay_ms(100);
	present = true;

	read(); // Flush buffer

	sendCMD(STR("AT\r")); // Check connection

	if(checkOK() == 0)
	{
		present = false;
		return 0;
	}

	// Set Name //
	sendCMD(STR("ATSN,Timelapse+\r")); // Set Name

	if(checkOK() == 0)
		return 0;

	sendCMD(STR("ATSZ,1,1,0\r")); // Configure Sleep mode (sleep on reset, wake on UART)

	if(checkOK() == 0)
		return 0;

	sendCMD(STR("ATSDIF,782,1,1\r")); // Configure discovery display

	if(checkOK() == 0)
		return 0;

	state = BT_ST_IDLE;
	mode = BT_MODE_CMD;

	devices = 0;
	newDevices = 0;

	return power(3);
}

/******************************************************************
 *
 *   BT::power
 *
 *
 ******************************************************************/

uint8_t BT::power(void)
{
	return btPower;
}

/******************************************************************
 *
 *   BT::power
 *
 *
 ******************************************************************/

uint8_t BT::power(uint8_t level)
{
	if(!present)
		return 1;

	switch(level)
	{
	   case 1:
		   sendCMD(STR("ATSPL,1,1\r"));
		   btPower = 1;
		   break;

	   case 2:
		   sendCMD(STR("ATSPL,2,1\r"));
		   btPower = 2;
		   break;

	   case 3:
		   sendCMD(STR("ATSPL,3,1\r"));
		   btPower = 3;
		   break;

	   default:
		   sendCMD(STR("ATSPL,0,0\r"));
		   btPower = 0;
		   break;
	}

	return checkOK();
}

/******************************************************************
 *
 *   BT::disconnect
 *
 *
 ******************************************************************/

uint8_t BT::disconnect(void)
{
	if(!present)
		return 1;

	sendCMD(STR("ATDH,0\r"));

	return checkOK();
}

/******************************************************************
 *
 *   BT::cancel
 *
 *
 ******************************************************************/

uint8_t BT::cancel(void)
{
	if(!present)
		return 1;

	sendCMD(STR("ATDC\r"));

	if(state != BT_ST_CONNECTED) state = BT_ST_IDLE;

	return checkOK();
}

/******************************************************************
 *
 *   BT::cancelScan
 *
 *
 ******************************************************************/

uint8_t BT::cancelScan(void)
{
	if(!present)
		return 1;

	sendCMD(STR("ATDC,1,1\r"));

	if(state != BT_ST_CONNECTED) state = BT_ST_IDLE;

	return checkOK();
}

/******************************************************************
 *
 *   BT::cmdMode
 *
 *
 ******************************************************************/

uint8_t BT::cmdMode(void)
{
	if(!present)
		return 1;

	if(mode == BT_MODE_DATA)
	{
		send(STR("+++\r"));
		if(checkOK())
		{
			mode = BT_MODE_CMD;
			return 1;
		}
		else
		{
			return 0;
		}
	}

	return 1;
}


/******************************************************************
 *
 *   BT::dataMode
 *
 *
 ******************************************************************/

uint8_t BT::dataMode(void)
{
	if(!present)
		return 1;

	if(state == BT_ST_CONNECTED)
	{
		if(mode == BT_MODE_CMD)
		{
			send(STR("ATMD\r"));
			if(checkOK())
			{
				mode = BT_MODE_DATA;
				return 1;
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}
	
	return 0;
}

/******************************************************************
 *
 *   BT::scan
 *
 *
 ******************************************************************/

uint8_t BT::scan(void)
{
	if(!present)
		return 1;

	if(state != BT_ST_CONNECTED) state = BT_ST_SCAN;

	newDevices = 0;

	sendCMD(STR("ATDILE\r"));

	return checkOK();
}

/******************************************************************
 *
 *   BT::scan
 *
 *
 ******************************************************************/

uint8_t BT::connect(char *address)
{
	if(!present)
		return 1;

	cancel();

	sendCMD(STR("ATDMLE"));
	sendCMD(address);
	sendCMD(STR("\r"));	

	return checkOK();
}

/******************************************************************
 *
 *   BT::sleep
 *
 *
 ******************************************************************/

uint8_t BT::sleep(void)
{
	if(!present)
		return 1;

	cancel();

	state = BT_ST_SLEEP;

	if(version() >= 3)
	{
		sendCMD(STR("ATZ\r"));

		return checkOK();
	}
	else
	{
		return 1;
	}
}

/******************************************************************
 *
 *   BT::temperature
 *
 *
 ******************************************************************/

uint8_t BT::temperature(void)
{
	if(!present)
		return 1;

	sendCMD(STR("ATT?\r"));

	uint8_t i = checkOK();
	uint8_t n = 0;

	if(i > 0)
	{
		for(++i; i < BT_BUF_SIZE; i++)
		{
			if(buf[i] == 0)
				break;

			if(buf[i] == ',')
			{
				n = (buf[i + 1] - '0') * 100;
				n += (buf[i + 2] - '0') * 10;
				n += (buf[i + 3] - '0');
				return n;
			}
		}
	}

	return 255;
}

/******************************************************************
 *
 *   BT::version
 *
 *
 ******************************************************************/

uint8_t BT::version(void)
{
	if(!present)
		return 1;

	sendCMD(STR("ATV?\r"));

	uint8_t i = checkOK();
	uint8_t n = 0;

	if(i > 0)
	{
		for(++i; i < BT_BUF_SIZE; i++)
		{
			if(buf[i] == 0)
				break;

			if(i == 14)
			{
				n = (buf[i] - '0');
				return n;
			}
		}
	}

	return 255;
}

/******************************************************************
 *
 *   BT::checkOK
 *
 *
 ******************************************************************/

uint8_t BT::checkOK(void)
{
	if(!present)
		return 1;

	_delay_ms(50);
	
	for(uint8_t i = 0; i < 3; i++)
	{
		read();
		if(strncmp(buf, STR("OK"), 2) == 0)
		{
			return 1;
		}
	}

	return 0;
}

/******************************************************************
 *
 *   BT::sendCMD
 *
 *
 ******************************************************************/

uint8_t BT::sendCMD(char* str)
{
	if(!present)
		return 1;

	cmdMode();

	return send(str);
}

/******************************************************************
 *
 *   BT::sendDATA
 *
 *
 ******************************************************************/

uint8_t BT::sendDATA(char* str)
{
	if(!present)
		return 1;

	if(dataMode())
	{
		return send(str);
	}

	return 0;
}

/******************************************************************
 *
 *   BT::send
 *
 *
 ******************************************************************/

uint8_t BT::send(char* str)
{
	if(!present)
		return 1;

	if(!(BT_RTS))
	{
		char i = 0;

		Serial_SendByte('A');

		while(!(BT_RTS))
		{
			if(++i > 250)
				break;

			_delay_ms(1);
		}
	}

	if(BT_RTS)
	{
		char* ptr;

		ptr = str;

		while(*ptr != 0)
		{
			Serial_SendByte(*ptr);
			ptr++;
		}
	}

	return 1;
}

/******************************************************************
 *
 *   BT::read
 *
 *
 ******************************************************************/

uint8_t BT::read(void)
{
	if(!present)
		return 1;


	uint8_t bytes = 0;

	BT_SET_CTS;

	uint16_t timeout;

	for(;;)
	{
		timeout = 0;

		while(!Serial_IsCharReceived())
		{
			if(++timeout > 6000)
				break;
		}

		if(timeout > 6000)
			break;

		buf[bytes] = (char)Serial_ReceiveByte();

		if(!(bytes == 0 && (buf[0] == '\n' || buf[0] == '\r'))) bytes++; // skip leading CR/LF

		if(bytes >= BT_BUF_SIZE)
			break;
		if(mode == BT_MODE_CMD && bytes > 0 && buf[bytes - 1] == '\r') // just get one line at a time
			break;
	}

	BT_CLR_CTS;

	buf[bytes] = 0;

	return bytes;
}


/******************************************************************
 *
 *   BT::task
 *
 *
 ******************************************************************/

uint8_t BT::task(void)
{
	if(!present)
		return 1;

	char *ptr;
	uint8_t pos = 0, len, ret = BT_EVENT_NULL;

	len = read();
	if(len)
	{
		if(mode == BT_MODE_CMD) debug(STR("CMD:\r\n")); else debug(STR("DATA:\r\n"));
		debug(buf);
		debug_nl();
		if(mode == BT_MODE_CMD)
		{
			ptr = buf;

			while(pos < len)
			{
				if(*(ptr + pos) == '\r' || *(ptr + pos) == '\n') // strip any leftover whitespace
				{
					pos++;
					continue;
				}

				if(strncmp(ptr + pos, STR("DISCOVERY"), 9) == 0)
				{
					if(*(ptr + pos + 10) == '6' && newDevices < BT_MAX_SCAN)
					{
						ret = BT_EVENT_DISCOVERY;
						uint8_t i;
						for(i = 0; i < BT_ADDR_LEN; i++)
						{
							device[newDevices].addr[i] = *(ptr + pos + 12 + i);
						}
						device[newDevices].addr[BT_ADDR_LEN - 1] = '\0';
						for(i = 0; i < BT_NAME_LEN; i++)
						{
							if(*(ptr + pos + 38 + i) == '\r') break;
							device[newDevices].name[i] = *(ptr + pos + 38 + i);
						}
						device[newDevices].name[i] = '\0';
						newDevices++;
						if(newDevices > devices) devices = newDevices;
					}
				}
				if(strncmp(ptr + pos, STR("CONNECT"), 7) == 0)
				{
					if(state == BT_ST_SCAN) cancelScan();
					ret = BT_EVENT_CONNECT;
					state = BT_ST_CONNECTED;
				}
				if(strncmp(ptr + pos, STR("BRSP"), 4) == 0)
				{
					ret = BT_EVENT_CONNECT;
					mode = BT_MODE_DATA;
					state = BT_ST_CONNECTED;
				}
				if(strncmp(ptr + pos, STR("DISCONNECT"), 10) == 0)
				{
					ret = BT_EVENT_DISCONNECT;
					mode = BT_MODE_CMD;
					state = BT_ST_IDLE;
				}
				if(strncmp(ptr + pos, STR("DONE"), 4) == 0)
				{
					ret = BT_EVENT_SCAN_COMPLETE;
					devices = newDevices;
				}

				while(pos < len && *(ptr + pos) != '\r') pos++; // jump ahead to next line
			}
		}
		else
		{
			if(strncmp(buf, STR("DISCONNECT"), 10) == 0)
			{
				ret = BT_EVENT_DISCONNECT;
				mode = BT_MODE_CMD;
				state = BT_ST_IDLE;
			}
			else
			{
				ret = BT_EVENT_DATA;
			}
		}
	}
	event = ret;
	return ret;
}

