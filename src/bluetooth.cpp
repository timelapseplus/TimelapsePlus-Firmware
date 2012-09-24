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
#include "hardware.h"
#include "bluetooth.h"


BT::BT(void)
{
  // Configure Bluetooth //
  BT_INIT_IO;
  Serial_Init(115200, true);
}

uint8_t BT::init(void)
{
  _delay_ms(100);
  present = true;

  read(); // Flush buffer

  send("AT\r"); // Check connection
  if(checkOK() == 0)
  {
    present = false;
    return 0;
  }

  // Set Name //
  send("ATSN,Timelapse+\r"); // Set Name

  if(checkOK() == 0) return 0;

  send("ATSZ,1,1,0\r"); // Configure Sleep mode (sleep on reset, wake on UART)

  if(checkOK() == 0) return 0;

  send("ATSDIF,782,1,1\r"); // Configure discovery display
  
  if(checkOK() == 0) return 0;

  return power(3);
}

uint8_t BT::power(void) { return btPower; } 

uint8_t BT::power(uint8_t level)
{
  if(!present) return 1;
  if(level == 1)
  {
    send("ATSPL,1,1\r");
    btPower = 1;
  }
  else if(level == 2)
  {
    send("ATSPL,2,1\r");
    btPower = 2;
  }
  else if(level == 3)
  {
    send("ATSPL,3,1\r");
    btPower = 3;
  }
  else
  {
    send("ATSPL,0,0\r");
    btPower = 0;
  }

  return checkOK();
}

uint8_t BT::cancel(void)
{
  if(!present) return 1;
  send("ATDC\r");
  return checkOK();
}

uint8_t BT::sleep(void)
{
  if(!present) return 1;
  cancel();
  if(version() >= 3)
  {
    send("ATZ\r");
    return checkOK();
  }
  else
  {
    return 1;
  }
}

uint8_t BT::temperature(void)
{
  if(!present) return 1;
  send("ATT?\r");
  uint8_t i = checkOK();
  uint8_t n = 0;
  if(i > 0)
  {
    for(++i; i < BT_DATA_SIZE; i++)
    {
      if(data[i] == 0) break;
      if(data[i] == ',')
      {
        n = (data[i + 1] - '0') * 100;
        n += (data[i + 2] - '0') * 10;
        n += (data[i + 3] - '0');
        return n;
      }
    }
  }
  return 255;
}

uint8_t BT::version(void)
{
  if(!present) return 1;
  send("ATV?\r");
  uint8_t i = checkOK();
  uint8_t n = 0;
  if(i > 0)
  {
    for(++i; i < BT_DATA_SIZE; i++)
    {
      if(data[i] == 0) break;
      if(i == 14)
      {
        n = (data[i] - '0');
        return n;
      }
    }
  }
  return 255;
}

uint8_t BT::checkOK(void)
{
  if(!present) return 1;
  _delay_ms(50);
  uint8_t bytes = read();
  for(uint8_t i = 0; i < bytes; i++)
  {
    if(data[i] == 0) break;
    if(data[i] == '\n' || data[i] == '\r') continue; // SKIP CR/LF
    if(data[i] == 'O' && data[i + 1] == 'K')
    {
      return i;
    }
  }
  return 0;
}

uint8_t BT::send(char *data)
{
  if(!present) return 1;
  if(!(BT_RTS))
  {
    Serial_SendByte('A');
    while(!(BT_RTS));
  }

  if(BT_RTS)
  {
    char *ptr;
    ptr = data;
    while(*ptr != 0)
    {
      Serial_SendByte(*ptr);
      ptr++;
    }
  }
}

uint8_t BT::read(void)
{
  if(!present) return 1;
  uint8_t bytes = 0;

  BT_SET_CTS;

  uint16_t timeout;
  for(;;)
  {
    timeout = 0;
    while(!Serial_IsCharReceived()) if(++timeout > 5000) break;
    if(timeout > 5000) break;
    data[bytes] = (char) Serial_ReceiveByte();
    bytes++;
    if(bytes >= BT_DATA_SIZE) break;
  }
  BT_CLR_CTS;

  data[bytes] = 0;
  return bytes;
}





