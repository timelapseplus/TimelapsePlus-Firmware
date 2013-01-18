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
#include <avr/wdt.h>
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
	debug(STR("BT Init\n\r"));

//	sendCMD(STR("ATRST\r")); // Reset module
//	_delay_ms(200);

	while(read()); // Flush buffer

	sendCMD(STR("AT\r")); // Check connection

	debug(STR("Data: "));
	debug(data);
	debug_nl();

	if(checkOK() == 0)
	{
		present = false;
		return 0;
	}

	sendCMD(STR("ATSDBLE,0,0\r")); // Default to Idle (don't advertise)

	if(checkOK() == 0)
		return 0;

	sendCMD(STR("ATSN,Timelapse+\r")); // Set Name

	if(checkOK() == 0)
		return 0;

	sendCMD(STR("ATSZ,1,1,0\r")); // Configure Sleep mode (sleep on reset, wake on UART)

	if(checkOK() == 0)
		return 0;

	sendCMD(STR("ATSDIF,782,1,1\r")); // Configure discovery display

	if(checkOK() == 0)
		return 0;

	sendCMD(STR("ATFC\r")); // Save configuration

	if(checkOK() == 0)
		return 0;

	debug(STR("BT Init: Saved configuration\r\n"));

	state = BT_ST_IDLE;
	mode = BT_MODE_CMD;

	devices = 0;
	newDevices = 0;

//	updateVersion();

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
 *   BT::advertise
 *
 *
 ******************************************************************/

uint8_t BT::advertise(void)
{
	if(!present)
		return 1;

	sendCMD(STR("ATDSLE\r"));

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

	read();

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
 *   BT::updateVersion
 *
 *
 ******************************************************************/

uint8_t BT::updateVersion(void)
{
	if(!present)
		return 1;

	sendCMD(STR("ATV?\r"));

	uint8_t i = checkOK();
	uint8_t n = 0;

	read();

	if(i > 0)
	{
		for(++i; i < BT_BUF_SIZE; i++)
		{
			if(buf[i] == 0)
				break;

			if(i == 6)
			{
				n = (buf[i] - '0');
				btVersion = n;
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
	updateVersion(); // disable caching for now
	return btVersion;
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
	
	for(uint8_t i = 0; i < 4; i++)
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

uint8_t BT::waitRTS()
{
	uint8_t i = 0;
	while(!(BT_RTS))
	{
		if(++i > 250)
		{
			debug(STR("BT RTS Failed!\r\n"));
			return 1;
		}
		_delay_ms(1);
	}
	return 0;
}

uint8_t BT::sendDATA(uint8_t id, uint8_t type, void* buffer, uint16_t bytes)
{
	if(!present)
		return 1;

	debug(STR("Sending: "));
	debug(id);
	debug(STR(", "));
	debug(type);
	debug_nl();
	if(dataMode())
	{
		waitRTS();
		if(BT_RTS)
		{
			char* byte;

			if(waitRTS()) return 1;
			Serial_SendByte('$');
			if(waitRTS()) return 1;
			Serial_SendByte((char) id);
			if(waitRTS()) return 1;
			Serial_SendByte((char) type);
			if(waitRTS()) return 1;
			Serial_SendByte((char) *(&bytes));
			if(waitRTS()) return 1;
			Serial_SendByte((char) *(&bytes + 1));
			if(waitRTS()) return 1;
			Serial_SendByte(':');

			byte = (char *) buffer;
			if(bytes > 0)
			{
				while(bytes--)
				{
					if(waitRTS()) break;
					Serial_SendByte(*byte);
					byte++;
				}
			}
			return 1;
		}
		else
		{
			debug(STR("BT RTS Failed!\r\n"));
		}
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

	char i = 0;
	if(!(BT_RTS) && state == BT_ST_SLEEP)
	{
		debug(STR("Waking up BT\r\n"));
		state = BT_ST_IDLE;
		while(!(BT_RTS))
		{
			Serial_SendByte('A');
			if(++i > 250)
			{
				state = BT_ST_SLEEP;
				break;
			}
			_delay_ms(1);
		}
		if(state == BT_ST_SLEEP)
		{
			debug(STR("ERROR: BT didn't wake up!\r\n"));
			return 0; // wakeup failed
		}
	}

	while(!(BT_RTS))
	{
		if(++i > 250) break;
		_delay_ms(1);
	}

	if(BT_RTS)
	{
		char* ptr;

		ptr = str;

		while(*ptr != 0)
		{
			while(!(BT_RTS))
			{
				if(++i > 250) break;
				_delay_ms(1);
			}
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


	uint16_t bytes = 0;

	dataId = 0;
	dataSize = 0;

	BT_SET_CTS;

	uint16_t timeout;

	for(;;)
	{
		timeout = 0;

		while(!Serial_IsCharReceived())
		{
			wdt_reset();
			_delay_us(10);
			if(++timeout > (bytes > 0 ? 50000 : 5000))
				break;
		}

		if(timeout > 50000)
		{
			debug(STR("TIMED OUT!"));
			debug(STR("\r\n   BYTES: "));
			debug(bytes);
			debug(STR("\r\n      ID: "));
			debug((uint8_t)buf[1]);
			debug(STR("\r\n    SIZE: "));
			debug(dataSize);
			debug(STR("\r\n  DATA[0]: "));
			debug(buf[0]);
			debug(STR("\r\n  DATA[1]: "));
			debug((uint8_t)buf[1]);
			debug(STR("\r\n  DATA[2]: "));
			debug((uint8_t)buf[2]);
			debug(STR("\r\n  DATA[3]: "));
			debug((uint8_t)buf[3]);
			debug(STR("\r\n  DATA[4]: "));
			debug(buf[4]);
			debug(STR("\r\n  DATA[5]: "));
			debug(buf[5]);
		    debug(STR("\r\n"));
			break;
		}
		else if(timeout > 5000 && bytes == 0)
		{
			break;
		}

		buf[bytes] = (char)Serial_ReceiveByte();

		if(!(bytes == 0 && (buf[0] == '\n' || buf[0] == '\r'))) bytes++; // skip leading CR/LF
		if(mode == BT_MODE_CMD)
		{
			if(bytes > 0 && buf[bytes - 1] == '\n') // just get one line at a time
				break;
		}
		else
		{
			if(buf[0] != '$' && bytes > 1 && buf[bytes - 1] == '$')
			{
				buf[0] = '$';
				bytes = 1;
			}
			if(dataSize > 0)
			{
				if(bytes > dataSize + 5) break;
			}
			else
			{
				if(bytes > 5 && buf[0] == '$' && buf[5] == ':')
				{
					dataId = buf[1];
					dataType = buf[2];

					*(&dataSize) = buf[3];
					*(&dataSize + 1) = buf[4];

					data = (buf + 6);

					if(dataSize == 0) break;
				}
				else if(buf[0] != '$' && buf[bytes - 1] == '\n') // just get one line at a time
				{
					break;
				}
			}
		}

		if(bytes >= BT_BUF_SIZE)
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

	uint8_t pos = 0, len, ret = BT_EVENT_NULL;

	len = read();

	if(len)
	{
		if(mode == BT_MODE_CMD) debug(STR("CMD:\r\n")); else debug(STR("DATA:\r\n"));
		debug(buf);
		debug_nl();
		if(mode == BT_MODE_CMD)
		{
			if(strncmp(buf, STR("DISCOVERY"), 9) == 0)
			{
				if(*(buf + 10) == '6' && newDevices < BT_MAX_SCAN)
				{
					ret = BT_EVENT_DISCOVERY;
					uint8_t i;
					for(i = 0; i < BT_ADDR_LEN; i++)
					{
						device[newDevices].addr[i] = *(buf + 12 + i);
					}
					device[newDevices].addr[BT_ADDR_LEN - 1] = '\0';
					for(i = 0; i < BT_NAME_LEN; i++)
					{
						if(*(buf + 38 + i) == '\r') break;
						device[newDevices].name[i] = *(buf + 38 + i);
					}
					device[newDevices].name[i] = '\0';
					newDevices++;
					if(newDevices > devices) devices = newDevices;
				}
			}
			if(strncmp(buf, STR("CONNECT"), 7) == 0)
			{
				if(state == BT_ST_SCAN) cancelScan();
				//ret = BT_EVENT_CONNECT;
				state = BT_ST_CONNECTED;
			}
			if(strncmp(buf, STR("BRSP"), 4) == 0)
			{
				ret = BT_EVENT_CONNECT;
				mode = BT_MODE_DATA;
				state = BT_ST_CONNECTED;
			}
			if(strncmp(buf, STR("DISCONNECT"), 10) == 0)
			{
				ret = BT_EVENT_DISCONNECT;
				mode = BT_MODE_CMD;
				state = BT_ST_IDLE;
			}
			if(strncmp(buf, STR("DONE"), 4) == 0)
			{
				ret = BT_EVENT_SCAN_COMPLETE;
				devices = newDevices;
			}
		}
		else
		{
			while(*(buf + pos) == '\r' || *(buf + pos) == '\n') // strip any leftover whitespace
			{
				pos++;
			}			
			if(strncmp(buf + pos, STR("DISCONNECT"), 10) == 0)
			{
				ret = BT_EVENT_DISCONNECT;
				mode = BT_MODE_CMD;
				state = BT_ST_IDLE;
			}
			else if(dataId > 0)
			{
				debug(STR("Received Packet: "));
				debug(dataId);
				debug(STR(" ("));
				debug(dataSize);
				debug(STR(" bytes)\r\n"));
				ret = BT_EVENT_DATA;
			}
		}
	}
	event = ret;
	return ret;
}

