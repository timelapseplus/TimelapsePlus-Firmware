/*
 *  Menu.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */
 
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>
#include "tldefs.h"
#include "5110LCD.h"
#include "button.h"
#include "clock.h"
#include "Menu.h"
#include "debug.h"
#include "settings.h"
#include "hardware.h"

extern Clock clock;
extern settings conf;

/******************************************************************
 *
 *   MENU::MENU
 *
 *
 ******************************************************************/

MENU::MENU()
{
    hx1 = 0;
    hy1 = 0;
    hx2 = 0;
    hy2 = 0;
    m_refresh = 0;
}

/******************************************************************
 *
 *   MENU::run
 *
 *
 ******************************************************************/

void MENU::task()
{
    char ret, key;
    static char first = 1;
    static unsigned int e_val;

    key = button->get();

    if(state == ST_CONT && (key == FL_KEY || key == FR_KEY))
    {
        if(handlerFunction)
        {
            //push();
            func = handlerFunction;
            state = ST_FUNC;
        }
    }

    if(m_refresh) first = 1;
    switch(state)
    {
       case ST_CONT:
           if(m_refresh) state = ST_MENU;
           switch(key)
           {
              case UP_KEY:
                  up();
                  break;

              case DOWN_KEY:
                  down();
                  break;

              case RIGHT_KEY:
                  click();
                  break;

              case LEFT_KEY:
                  back();
                  break;
           }
           break;

       case ST_MENU:
           first = 1;
           init(menu);
           lcd->update();
           break;

       case ST_EDIT:
           ret = editNumber(key, var, name, desc, type, first);
           first = 0;
           
           if(ret != FN_CONTINUE) 
           {
               if(ret == FN_SAVE && func_short)
               {
                   (*func_short)();
               }
               state = ST_MENU;
           }
           break;

       case ST_TEXT:
           ret = editNumber(key, var, name, desc, type, first);
           first = 0;
           
           if(ret != FN_CONTINUE)
           {
               if(ret == FN_SAVE && func_short)
               {
                   (*func_short)();
               }
               state = ST_MENU;
           }
           break;

       case ST_LIST:
           ret = editSelect(key, bvar, list, name, first);
           first = 0;
           if(ret != FN_CONTINUE)
           {
               if(ret == FN_SAVE && func_short)
               {
                   (*func_short)();
               }
               state = ST_MENU;
           }
           break;

       case ST_DLIST:
           ret = editDynamic(key, (uint8_t *)bvar, dlist, name, first);
           first = 0;
           if(ret != FN_CONTINUE)
           {
               if(ret == FN_SAVE && func_short)
               {
                   (*func_short)();
               }
               state = ST_MENU;
           }
           break;

       case ST_EEPR:
           if(first)
           {
               eeprom_read_block(&e_val, var, 2);
           }
           ret = editNumber(key, &e_val, name, desc, type, first);
           first = 0;
           
           if(ret != FN_CONTINUE)
           {
               if(ret == FN_SAVE)
               {
                   unsigned int tmp;
                   eeprom_read_block(&tmp, var, 2);
                   
                   if(tmp != e_val) 
                       eeprom_write_block(&e_val, var, 2);
               }
               state = ST_MENU;
           }
           break;

       case ST_FUNC:
           ret = (*func)(key, first);
           first = 0;
           
           if(ret != FN_CONTINUE)
           {
                first = 1;
                if(state == ST_FUNC) state = ST_MENU;
                /*if(ret == FN_JUMP)
                {
                  stack_counter--;
                }
                else
                {
                  back();
                }*/
           }
           break;

       case ST_SPAWN:
            first = 1;
            state = ST_FUNC;
            break;

       case ST_SUBMENU:
            first = 1;
            state = ST_MENU;
            break;

       case ST_ALERT_NEW:
            first = 1;
            state = ST_ALERT;
       case ST_ALERT:
            if(alert_index > 0 && alerts[alert_index - 1])
            {
              ret = alertTask(key, first);
            }
            else
            {
              DEBUG(STR("Exiting alerts\n"));
              ret = FN_CANCEL;
            }
            key = 0;
            first = 0;
            if(ret != FN_CONTINUE)
            {
              if(alert_index > 0) alert_index--;
              first = 1;
              if(alert_index == 0)
              {
                state = alert_return_state;
              }
            }
            break; 
    }

    if(state != ST_CONT) 
        key = 0;

    m_refresh = 0;


    if(message_text) // For displaying brief pop-up messages
    {
        if(message_time == 0)
        {
            message_time = clock.Ms();
        }
        uint8_t l = strlen(message_text) * 6 / 2;
        lcd->eraseBox(41 - l - 2, 12, 41 + l + 2, 24);
        lcd->drawBox(41 - l - 1, 13, 41 + l + 1, 23);
        lcd->writeString(41 - l, 15, message_text);
        lcd->disableUpdate = 0;
        lcd->update();
        lcd->disableUpdate = 1;

        if((clock.Ms() - message_time >= MENU_MESSAGE_DISPLAY_TIME))
        {
            lcd->disableUpdate = 0;
            message_text = 0;
            message_time = 0;
            refresh();
        } 
    }

    unusedKey = key;
}


/******************************************************************
 *
 *   MENU::init
 *
 *
 ******************************************************************/

void MENU::init(menu_item *newmenu)
{
    uint8_t i;
    uint8_t c;
    unsigned char ch, var_len = 0;
    
    menu = newmenu;
    clearHighlight();
    lcd->cls();

    checkScroll();
    state = ST_CONT;  // We're back in the menu system //

    menuSize = 0;
    
    for(i = 0; i < MENU_MAX; i++)
    {
        char *condition;
        
        condition = (char*)pgm_read_word(&menu[i].condition);

        if(!condition || *condition)
        {
            type = pgm_read_byte(&menu[i].type);
            c = pgm_read_byte(&menu[i].name[MENU_NAME_LEN - 2]);

            if(type == 'E' || type == 'P')  // Edit variable type //
            {
                unsigned int *var;
                unsigned int val;

                var = (unsigned int*)pgm_read_word(&menu[i].function);
                
                if(type == 'P')
                {
                    eeprom_read_block(&val, var, 2);
                    var = &val;
                }
                
                if(type != 'C') var_len = lcd->writeNumber(2 + MENU_NAME_LEN * 6, 8 + 9 * menuSize - menuScroll, *var, c, 'R',false);  //J.R. 2-27-14
            }

            if(type == 'S' && c == '*') // Display setting selection in place of menu text
            {
                settings_item *set;
                set = (settings_item*)pgm_read_word(&menu[i].function);
                var = (unsigned int*)pgm_read_word(&menu[i].description);

                for(uint8_t x = 0; x < MENU_MAX; x++)
                {
                    if(pgm_read_byte(&set[x].name[0]) == '0') 
                        break;
                    
                    if(pgm_read_byte(&set[x].value) == *var)
                    {
                        for(c = 0; c < MENU_NAME_LEN - 1; c++) // Write settings item text //
                        {
                            ch = pgm_read_byte(&set[x].name[c]);
                            lcd->writeChar(2 + c * 6, 8 + 9 * menuSize - menuScroll, ch);
                        }
                        break;
                    }
                }
            } 
            else
            {
                for(uint8_t b = 0; b < MENU_NAME_LEN - 1; b++) // Write menu item text //
                {
                    if((type != 'E' && type != 'P') || b < MENU_NAME_LEN - var_len - 1)
                    {
                        ch = pgm_read_byte(&menu[i].name[b]);
                        if(ch == '+') continue;
                        lcd->writeChar(2 + b * 6, 8 + 9 * menuSize - menuScroll, ch);
                    }
                }
                if(type == 'M')
                {
                  lcd->writeChar(2 + (MENU_NAME_LEN - 1) * 6, 8 + 9 * menuSize - menuScroll, '>');
                }
                ch = pgm_read_byte(&menu[i].name[MENU_NAME_LEN - 2]);                                             
                if(type == 'C' && (ch == 'M' || ch == 'N' || ch == 'L'))  //Correction for Bramp Min-Max J.R.
                {
                  //lcd->writeChar(2 + (MENU_NAME_LEN - 2) * 6, 8 + 9 * menuSize - menuScroll, '*');
                  lcd->eraseBox(2 + (MENU_NAME_LEN - 2) * 6, 8 + 9 * menuSize - menuScroll, 8 + (MENU_NAME_LEN - 2) * 6, 16 + 9 * menuSize - menuScroll);  //J.R.  
                }
            }
            
            if(type == 'S' && c == '+') // Write setting selection over menu text
            {
                settings_item *set;
                set = (settings_item*)pgm_read_word(&menu[i].function);
                var = (unsigned int*)pgm_read_word(&menu[i].description);

                for(uint8_t x = 0; x < MENU_MAX; x++)
                {
                    if(pgm_read_byte(&set[x].name[0]) == '0') 
                        break;
                    
                    if(pgm_read_byte(&set[x].value) == *var)
                    {
                        for(c = 0; c < MENU_NAME_LEN - 1; c++) // Write settings item text //
                        {
                            ch = pgm_read_byte(&set[x].name[c]);
                            lcd->writeChar(2 + (c + 2) * 6, 8 + 9 * menuSize - menuScroll, ch);
                        }
                        break;
                    }
                }
            }
            else if(type == 'D' && c == '+') // Write setting selection over menu text
            {
                dynamicItem_t *set;
                set = (dynamicItem_t*)pgm_read_word(&menu[i].function);
                var = (unsigned int*)pgm_read_word(&menu[i].description);

                char item_name[8];
                uint8_t (*nameFunc)(char name[8], uint8_t id);
                nameFunc = (uint8_t (*)(char*, uint8_t))pgm_read_word(&set->nameFunc);
                if((nameFunc)(item_name, *var))
                {
                  for(c = 0; c < 7; c++) // Write item text //
                  {
                    if(item_name[c] == 0) break;
                    lcd->writeChar(2 + (c + 6) * 6, 8 + 9 * menuSize - menuScroll, item_name[c]);
                  }        
                }
            }
            menuSize++;
        }
        
        ch = pgm_read_byte(&menu[i + 1]);
        if(ch == 0) break;
    }
    
    i++;
    select(menuSelected);

    char str[MENU_NAME_LEN];

    if(pgm_read_byte(&menu[i].name[1]) != ' ')
    {
        strcpy_P(str, (const char*)&menu[i].name[1]);
    } 
    else
    {
        menuName(str);
    }

    setTitle(str);
    ch = pgm_read_byte(&menu[i].type);
    handlerFunction = 0;

    if(ch == 'B' || ch == 'F')
    {
        char l[10], r[10];
        strcpy_P(l, (const char*)pgm_read_word(&menu[i].description));
        strcpy_P(r, (const char*)pgm_read_word(&menu[i].condition));
        setBar(l, r);

        if(ch == 'F') 
            handlerFunction = (char (*)(char, char))pgm_read_word(&menu[i].function);
    }
}

/******************************************************************
 *
 *   MENU::setTitle
 *
 *
 ******************************************************************/

void MENU::setTitle(char *title)
{
    char l = lcd->measureStringTiny(title) / 2;
    
    lcd->eraseBox(0, 0, 83, 5);
    lcd->drawHighlight(0, 1, 83, 3);
    lcd->eraseBox(41 - l - 1, 0, 41 + l + 1, 5);
    lcd->writeStringTiny(41 - l, 0, title);

/*  lcd->eraseBox(0, 0, 83, 6);
  lcd->writeStringTiny(2, 1, title);
  lcd->drawHighlight(0, 0, 83, 6);
  lcd->clearPixel(0, 0);
  lcd->clearPixel(83, 0);
  lcd->clearPixel(0, 6);
  lcd->clearPixel(83, 6); */
}

/******************************************************************
 *
 *   MENU::setBar
 *
 *
 ******************************************************************/

void MENU::setBar(char *left, char *right)
{
    lcd->eraseBox(0, 41, 83, 47);
    lcd->writeStringTiny(2, 42, left);
    lcd->writeStringTiny(83 - lcd->measureStringTiny(right), 42, right);
    lcd->drawHighlight(0, 41, 83, 47);
/*  lcd->clearPixel(0, 41);
  lcd->clearPixel(83, 41);
  lcd->clearPixel(0, 47);
  lcd->clearPixel(83, 47); */
}

/******************************************************************
 *
 *   MENU::getIndex
 *
 *
 ******************************************************************/

uint8_t MENU::getIndex(menu_item *cmenu, uint8_t selected)
{
    uint8_t index = 0, i = 0;

    while (index < MENU_MAX) // determine index //
    {
        char *condition;
        
        condition = (char*)pgm_read_word(&cmenu[index].condition);
        
        if(!condition || *condition)
        {
            if(selected == i) 
                break;
            
            i++;
        }
        
        index++;
    }
    
    return index;
}

/******************************************************************
 *
 *   MENU::getSelected
 *
 *
 ******************************************************************/

uint8_t MENU::getSelected(menu_item *cmenu, uint8_t index)
{
    uint8_t ind = 0, i = 0;

    while (ind < MENU_MAX) // determine index //
    {
        uint8_t *condition;
        condition = (uint8_t*)pgm_read_word(&cmenu[ind].condition);
        
        if(index == ind) 
            break;
        
        if(!condition || *condition)
        {
            i++;
        }
        
        ind++;
    }

    return i;
}

/******************************************************************
 *
 *   MENU::menuName
 *
 *
 ******************************************************************/

char *MENU::menuName(char *str)
{
    uint8_t index = 0, i = 0;

    if(stack_counter > 0) // is there a calling menu? //
    {
        if(stack[stack_counter - 1].type == 0)
        {
          menu_item *cmenu = (menu_item*)stack[stack_counter - 1].item; // retreive calling menu //

          index = stack[stack_counter - 1].index;;

          uint8_t j = 0;

          for(i = 0; i < MENU_NAME_LEN - 1; i++) // read name //
          {
              char c = pgm_read_byte(&cmenu[index].name[i]);
              if(j == 0 &&  (c == ' ' || c == '-')) continue;
              *(str + j) = pgm_read_byte(&cmenu[index].name[i]);
              j++;
          }
          
          for(i = j - 1; i > 1; i--) // trim //
          {
              if(*(str + i) != ' ')
              {
                  *(str + i + 1) = '\0';
                  break;
              }
          }
        }
        else
        {
          strcpy_P(str, PSTR("OPTIONS"));
        }
    } 
    else
    {
      strcpy_P(str, PSTR("MAIN MENU"));
    }

    return str;
}

/******************************************************************
 *
 *   MENU::click
 *
 *
 ******************************************************************/

void MENU::click()
{
    if(menuSelected >= 0 && menuSelected < menuSize)
    {
        uint8_t index = 0;

        index = getIndex(menu, menuSelected);

        type = pgm_read_byte(&menu[index].type);
        
        switch(type)
        {
           case 'M': // Submenu
               push();
               func_short = (char (*)(void))pgm_read_word(&menu[index].short_function);
               if(func_short) (*func_short)();
               menu = (menu_item*)pgm_read_word(&menu[index].function);
               select(0);
               state = ST_MENU;
               break;

           case 'F': // Function
               //push();
               func = (char (*)(char, char))pgm_read_word(&menu[index].function);
               state = ST_FUNC;
               break;

           case 'C':
           case 'E':
           case 'P': // Edit Variable
               {
                   unsigned char *desc_addr;

                   uint8_t b = 0, i = 0;

                   while(b < MENU_NAME_LEN - 1)		//J.R.
                   {
                       name[b] = pgm_read_byte(&menu[index].name[i]);
                       i++;
                       if(b == 0 && (name[b] == ' ' || name[b] == '-')) continue;

                       if(name[b] == ' ' && b > 8) //J.R. 3-7-14
                           break;
                       if(name[b] == ' ' && name[b-1] == ' ') //J.R. 8-21-14
						   {name[b-1] = '\0'; break;  }                                                   
                       b++;
                   }
                   
                   name[b] = '\0';

                   desc_addr = (unsigned char (*))pgm_read_word(&menu[index].description);
                   b = 0;

                   if(desc_addr > 0)
                   {
                       while(b < MENU_NAME_LEN)
                       {
                           desc[b] = pgm_read_byte(desc_addr + b);

                           if(desc[b] == '\0') 
                               break;

                           b++;
                       }
                   }
                   
                   desc[b] = '\0';

                   if(type == 'P') 
                       state = ST_EEPR;
                   else 
                       state = ST_EDIT;

                   func_short = (char (*)(void))pgm_read_word(&menu[index].short_function);
                   type = pgm_read_byte(&menu[index].name[MENU_NAME_LEN - 2]);
                   var = (unsigned int (*))pgm_read_word(&menu[index].function);
                   break;
               }

           case 'S': // Settings List Variable
           case 'D': // Dynamic Settings List Variable
               uint8_t b = 0, i = 0;
               
               while(b < MENU_NAME_LEN - 1)
               {
                   name[b] = pgm_read_byte(&menu[index].name[i]);
                   i++;
                   if(b == 0 && (name[b] == ' ' || name[b] == '-')) continue;

                   if(name[b] == ' ' && b > 8) //J.R. 5-30-14
                       break;
                   if(name[b] == ' ' && name[b-1] == ' ') //J.R. 8-21-14
						{name[b-1] = '\0'; break;  }                     
                   b++;
               }
               
               name[b] = '\0';

               func_short = (char (*)(void))pgm_read_word(&menu[index].short_function);
               bvar = (char (*))pgm_read_word(&menu[index].description);
               if(type == 'D')
               {
                 dlist = (dynamicItem_t(*)) pgm_read_word(&menu[index].function);
                 state = ST_DLIST;
               }
               else
               {
                 list = (settings_item(*)) pgm_read_word(&menu[index].function);
                 state = ST_LIST;
               }
               break;
        }
    }
}


/******************************************************************
 *
 *   MENU::spawn
 *
 *
 ******************************************************************/

void MENU::spawn(void *function)
{
   func = (char (*)(char, char)) function;
   state = ST_SPAWN;
}

/******************************************************************
 *
 *   MENU::submenu
 *
 *
 ******************************************************************/

void MENU::submenu(void *new_menu)
{
    menuSelected = 0;
    menu = (menu_item*)new_menu;
    checkScroll();
    state = ST_SUBMENU;
}

/******************************************************************
 *
 *   MENU::refresh
 *
 *
 ******************************************************************/

void MENU::refresh()
{
   m_refresh = 1;
}

/******************************************************************
 *
 *   MENU::back
 *
 *
 ******************************************************************/

void MENU::back()
{
    if(stack_counter > 0)
    {
        DEBUG(STR("POP: "));
        DEBUG(stack_counter);
        menu_stack new_menu;
        new_menu = menu_pop();
        if(new_menu.type == 1)
        {
          DEBUG(STR(" (function)\r\n"));
          func = (char (*)(char, char)) new_menu.item;
          menuSelected = getSelected((menu_item*)new_menu.menu_backup, new_menu.index);
          menu = (menu_item*)new_menu.menu_backup;
          checkScroll();
          state = ST_SPAWN;
        }
        else
        {
          DEBUG(STR(" (menu)\r\n"));
          menuSelected = getSelected((menu_item*)new_menu.item, new_menu.index);
          menu = (menu_item*)new_menu.item;
          state = ST_MENU;
        }
    }
    else
    {
        DEBUG(STR("POP: BOTTOM\r\n"));
    }
}

/******************************************************************
 *
 *   MENU::back
 *
 *
 ******************************************************************/

void MENU::clearStack()
{
  stack_counter = 0;
  m_refresh = 1;
}

/******************************************************************
 *
 *   MENU::highlight
 *
 *
 ******************************************************************/

void MENU::highlight(char x1, char y1, char x2, char y2)
{
    if(hx1 | hy1 | hx2 | hy2) 
        lcd->drawHighlight(hx1, hy1, hx2, hy2);

    hx1 = x1;
    hy1 = y1;
    hx2 = x2;
    hy2 = y2;

    lcd->drawHighlight(hx1, hy1, hx2, hy2);
}

/******************************************************************
 *
 *   MENU::clearHighlight
 *
 *
 ******************************************************************/

void MENU::clearHighlight()
{
    if(hx1 | hy1 | hx2 | hy2) 
        lcd->drawHighlight(hx1, hy1, hx2, hy2);

    hx1 = 0;
    hy1 = 0;
    hx2 = 0;
    hy2 = 0;
}

/******************************************************************
 *
 *   MENU::select
 *
 *
 ******************************************************************/

void MENU::select(char menuItem)
{
    if(menuItem < 0 || menuItem >= menuSize)
    {
        clearHighlight();
        menuSelected = -1;
    }
    else
    {
        highlight(1, 7 + (9 * menuItem) - menuScroll, 3 + 6 * 13, 15 + (9 * menuItem) - menuScroll);
        menuSelected = menuItem;
    }
}

/******************************************************************
 *
 *   MENU::down
 *
 *
 ******************************************************************/

void MENU::down()
{
    if(menuSelected < menuSize - 1) 
        select(menuSelected + 1);
    else
        if(conf.menuWrap) select(0);
    
    if(checkScroll()) 
        init(menu);

    lcd->update();
}

/******************************************************************
 *
 *   MENU::up
 *
 *
 ******************************************************************/

void MENU::up()
{
    if(menuSelected > 0) 
        select(menuSelected - 1);
    else
        if(conf.menuWrap) select(menuSize - 1);

    if(checkScroll()) 
        init(menu);
    
    lcd->update();
}

/******************************************************************
 *
 *   MENU::editNumber
 *
 *
 ******************************************************************/

char MENU::editNumber(char key, unsigned int *n, char *name, char *unit, char mode, char first)
{
    static char t;
    static uint8_t i;
    static char ret;
    static int c;
    static uint8_t l;
    static uint8_t x;
    static char d[5];
    unsigned int m;

    ret = FN_CONTINUE;

    if(first)
    {
        m = *n;
        lcd->cls();

        // Draw frame //
        lcd->drawHighlight(0, 2, 83, 5); // top //
        lcd->drawHighlight(0, 6, 1, 30); // left //
        lcd->drawHighlight(82, 6, 83, 30); // right //
        l = strlen(unit);
        lcd->writeString(80 - l * 6, 32, unit);

        l = strlen(name);
        if(l > 11) {l = 11; *(name + 11) = '\0';}	//J.R. 8-21-14
        x = 42 - ((l * 6) >> 1);
        lcd->drawHighlight(0, 31, 83, 39); // bottom //

        setBar(TEXT("CANCEL"), TEXT("SAVE"));

        lcd->drawHighlight(x, 2, x + l * 6, 5); // erases name space //
        lcd->writeString(x, 0, name);

        // Number -> Chars //
        
        for(i = 0; i < 5; i++) 
            d[i] = 0;

        switch(mode)
        {
           case 'F':
               // seconds //
               c = m % 600;
               m -= (unsigned int)c; m /= 600;
               d[0] = c % 10;
               c -= d[0]; c /= 10;
               d[1] = c % 10;
               c -= d[1]; c /= 10;
               d[2] = c;

               // minutes //
               c = m % 60;
               d[3] = c % 10;
               c -= d[3]; c /= 10;
               d[4] = c;
               break;

           case 'T':
               // seconds //
               c = m % 60;
               m -= (unsigned int)c; m /= 60;
               d[0] = c % 10;
               c -= d[0]; c /= 10;
               d[1] = c;

               // minutes //
               c = m % 60;
               m -= (unsigned int)c; m /= 60;
               d[2] = c % 10;
               c -= d[2]; c /= 10;
               d[3] = c;

               // hours //
               c = m % 60;
               m -= (unsigned int)c; m /= 60;
               d[4] = c % 10;
               break;
               
           case 'H':  	  //  Added case for hh:mm time frame - J.R.

               // minutes //
               c = m % 60;
               m -= (unsigned int)c; m /= 60;
               d[0] = c % 10;
               c -= d[0]; c /= 10;
               d[1] = c;

               // hours //
               c = m % 100;
               d[2] = c % 10;
               c -= d[2]; c /= 10;
               d[3] = c;
               break;              

           default:
               l = 0;
               
               while (m > 0)
               {
                   c = m % 10;
                   m -= (unsigned int)c; m /= 10;
                   d[l] = c;
                   l++;
               }
               break;
        }

        l = 5; // Number of editable digits //
        i = 0;
    }
    
    if(key != FL_KEY && key != FR_KEY)
    {

        lcd->eraseBox(3, 8, 81, 30);
        // Number -> Chars //
        for(x = 0; x < l; x++)
        {
            lcd->writeCharBig(67 - x * 16, 7, d[x] + '0');
        }      
        switch(mode)
        {
           case 'F': // Float (4.1) //
                lcd->drawBox(68 - 2 * 16 - 3, 12, 68 - 2 * 16 - 2, 14); // Colon (:) //
                lcd->drawBox(68 - 2 * 16 - 3, 20, 68 - 2 * 16 - 2, 22);
    
                lcd->drawBox(68 - 0 * 16 - 3, 24, 68 - 0 * 16 - 2, 26); // Decimal Point (.) //
                break;
                
           case 'T': // Time //
                lcd->drawBox(68 - 1 * 16 - 3, 12, 68 - 1 * 16 - 2, 14); // Colon (:) //
                lcd->drawBox(68 - 1 * 16 - 3, 20, 68 - 1 * 16 - 2, 22);
                lcd->drawBox(68 - 3 * 16 - 3, 12, 68 - 3 * 16 - 2, 14);
                lcd->drawBox(68 - 3 * 16 - 3, 20, 68 - 3 * 16 - 2, 22);
                break;
                
           case 'H': // Time (hours)//
                lcd->drawBox(68 - 1 * 16 - 3, 12, 68 - 1 * 16 - 2, 14); // Colon (:) //
                lcd->drawBox(68 - 1 * 16 - 3, 20, 68 - 1 * 16 - 2, 22);
                break;
           
           case 'L': // negative number J.R.//
				lcd->eraseBox(67 - 3 * 16, 8, 67 - 2 * 16, 30);
				if(conf.negBulbOffset) lcd->writeCharBig(67 - 3 * 16, 7, '-');              
        }

        lcd->drawHighlight(68 - (i * 16) - 1, 8, 68 - (i * 16) + 12, 29);

        switch(mode)
        {
           case 'F':
               if(i == 2 || i == 4) 
                   t = 5;
               else 
                   t = 9;
               break;
               
           case 'T':
               if(i % 2) 
                   t = 5;
               else
                   t = 9;
               break;
               
           case 'H':		//  Added case for hh:mm
				   l = 4;
               if(i == 1) 
                   t = 5;
               else				   
                   t = 9;
               break;
           case 'N':
           case 'M':		//J.R. Added case for Bramp Max/Min
				   l = 2;
			   if(i == 1)
			   {
				   if(mode == 'N') t = 2; 	//case = 'N'  - Bramp Min	
				   else 		   t = 5;	//case = 'M'  - Bramp Max
				   if(d[1] == 5 && mode == 'M') d[0] = 0;		
				   if(d[1] == 2 && mode == 'N') d[0] = 0;
			   }
			   else
			   {
				   t = 9; 
				   if(d[0] == 1 && d[1] == 5 && mode == 'M') d[1] = 0;		
				   if(d[0] == 1 && d[1] == 2 && mode == 'N') d[1] = 0;
				   if(d[0] == 9 && d[1] == 5 && mode == 'M') d[1] = 4;		
				   if(d[0] == 9 && d[1] == 2 && mode == 'N') d[1] = 1;		   
				}	    			   										   
			   break;
			   
           case 'L':		//  Added case for Bulb Offset  J.R.
				   l = 4;
               if(i == 3) 
                   t = 0;
               else				   
                   t = 9;
               break;			   
			   			   
			   
           default:
               if(i == 4) 
                   t = 5;
               else 
                   t = 9;
               break;
        }

        lcd->update();

        if(key != 0)
        {
            switch(key)
            {
               case RIGHT_KEY:
                    lcd->drawHighlight(68 - (i * 16) - 1, 8, 68 - (i * 16) + 12, 28);

                    if(i > 0) 
                        i--;
                    else 
                        i = l - 1;
                    break;
                    
               case LEFT_KEY:
                   lcd->drawHighlight(68 - (i * 16) - 1, 8, 68 - (i * 16) + 12, 28);

                   if(i < l - 1) 
                       i++;
                   else 
                       i = 0;
                   break;
                   
               case DOWN_KEY:
                   if(d[i] > 0) 
                       d[i]--;
                   else 
                       d[i] = t;
                   break;
                   
               case UP_KEY:
                   if(d[i] < t) 
                       d[i]++;
                   else 
                       d[i] = 0;
                   break;
            }
            if(mode == 'L' && i == 3)   //J.R.
				if(key == UP_KEY || key == DOWN_KEY)
				{
					if(conf.negBulbOffset) conf.negBulbOffset = 0;
					else conf.negBulbOffset = 1;
				}
        }
    } else
    {
        m = 0;
        
        switch(mode)
        {
           case 'F':
                m += (d[4] * 10 + d[3]) * 600; // minutes
                m += d[2] * 100 + d[1] * 10 + d[0]; // seconds
                DEBUG(m);
                DEBUG_NL();
                break;
            
           case 'T':
               m += d[4] * 3600;  // hours
               m += (d[3] * 10 + d[2]) * 60; // minutes
               m += d[1] * 10 + d[0]; // seconds
               DEBUG(m);
               DEBUG_NL();
               break;
               
		   case 'H':		//  Added case for hh:mm
                m += (d[3] * 10 + d[2]) * 60; // hours
                m += d[1] * 10 + d[0]; // minutes
                DEBUG(m);
                DEBUG_NL();
                break; 
                
           case 'L':		//  Added case for Bulb Offset  J.R.
                m = d[2];
                m *= 10;
                m += d[1];
                m *= 10;
                m += d[0];
                break;	                
                            
                             
           default:
                m = d[4];
                m *= 10;
                m += d[3];
                m *= 10;
                m += d[2];
                m *= 10;
                m += d[1];
                m *= 10;
                m += d[0];
                break;
        }

        switch(key)
        {
           case FR_KEY:
                message(TEXT("Saved"));
                *n = m; // Save new Value if not cancelled //
                ret = FN_SAVE;
                break;
                
           case FL_KEY:
               ret = FN_CANCEL;
               break;
        }
    }

    return ret;
}

/******************************************************************
 *
 *   MENU::message
 *
 *
 ******************************************************************/

void MENU::message(char *m)
{
    message_text = m;
    message_time = 0;
}

/******************************************************************
 *
 *   MENU::push
 *
 *
 ******************************************************************/

void MENU::push()
{
    if(state == ST_FUNC)
    {
      menu_push((void*)func, 0, 1);
    }
    else
    {
      menu_push(menu, menuSelected, 0);
    }
}

/******************************************************************
 *
 *   MENU::push
 *
 *
 ******************************************************************/

void MENU::push(uint8_t type)
{
    if(type == 1)
    {
      menu_push((void*)func, 0, 1);
    }
    else
    {
      menu_push(menu, menuSelected, 0);
    }
}

/******************************************************************
 *
 *   MENU::menu_push
 *
 *
 ******************************************************************/

void MENU::menu_push(void *item_addr, char selection, uint8_t type)
{
    if(stack_counter < MENU_STACK_SIZE)
    {
        stack_counter++;
        stack[stack_counter - 1].type = type;
        stack[stack_counter - 1].item = item_addr;
        if(type == 0)
        {
          stack[stack_counter - 1].index = getIndex((menu_item*)item_addr, selection);
        }
        else
        {
          stack[stack_counter - 1].index = getIndex((menu_item*)menu, menuSelected);
          stack[stack_counter - 1].menu_backup = (void*)menu;
        }
        DEBUG(STR("PUSH: "));
        DEBUG(stack_counter);
        if(type == 1)
        {
          DEBUG(STR(" (function)\r\n"));
        }
        else
        {
          DEBUG(STR(" (menu)\r\n"));
        }
    }
    else
    {
      DEBUG(STR("ERROR: Menu Stack Full!\r\n"));
    }
}

/******************************************************************
 *
 *   MENU::menu_pop
 *
 *
 ******************************************************************/

menu_stack MENU::menu_pop()
{
    if(stack_counter > 0)
    {
        stack_counter--;
        return stack[stack_counter];
    }

    // todo need to return a proper error indicator -- John
    return stack[0];
}

/******************************************************************
 *
 *   MENU::checkScroll
 *
 *
 ******************************************************************/

char MENU::checkScroll()
{
    char was = menuScroll;
    
    if(menuSelected > 2)
    {
        if(menuSelected == menuSize - 1)
        {
            menuScroll = (menuSize - 1) * 9 - 23;
        } 
        else
        {
            menuScroll = (menuSelected - 2) * 9;
        }
    } 
    else
    {
        menuScroll = 0;
    }

    return ((menuScroll > 0) || (was > 0));
}

/******************************************************************
 *
 *   MENU::editSelect
 *
 *
 ******************************************************************/

char MENU::editSelect(char key, char *n, void *settingslist, char *name, char first)
{
    settings_item *slist;
    static uint8_t i, l;
    uint8_t c = 0;
    char ch = 0;
    
    slist = (settings_item*)settingslist;

    if(first)
    {
        lcd->cls();
        setTitle(name);
        setBar(TEXT("CANCEL"), TEXT("SAVE"));

        // Box //
        lcd->drawBox(2, 12, 71, 24);
        lcd->drawBox(1, 11, 72, 25);

        // Up Arrow //
        lcd->drawLine(74, 17, 82, 17);
        lcd->drawLine(74, 16, 82, 16);
        lcd->drawLine(75, 15, 81, 15);
        lcd->drawLine(76, 14, 80, 14);
        lcd->drawLine(77, 13, 79, 13);
        //   lcd->setPixel(78, 12);

        // Down Arrow //
        lcd->drawLine(74, 19, 82, 19);
        lcd->drawLine(74, 20, 82, 20);
        lcd->drawLine(75, 21, 81, 21);
        lcd->drawLine(76, 22, 80, 22);
        lcd->drawLine(77, 23, 79, 23);
//    lcd->setPixel(78, 24);

        // Find current index //
        i = 0;
        l = 0;
        
        while (pgm_read_byte(&slist[l].name[0]) != 0)
        {
            if(*n == pgm_read_byte(&slist[l].value)) 
                i = l;
            
            l++;
        }
    }

    switch(key)
    {
       case UP_KEY:
           if(i > 0) i--;
           ch = 1;
           break;
           
       case DOWN_KEY:
           if(i < l - 1) i++;
           ch = 1;
           break;
    }

    if(ch || first)
    {
        lcd->eraseBox(3, 13, 69, 23);

        for(c = 0; c < MENU_NAME_LEN - 2; c++) // Write menu item text //
        {
            ch = pgm_read_byte(&slist[i].name[c]);
            lcd->writeChar(4 + c * 6, 15, ch);
        }
        
        c = 0;
        char *tmp;
        uint8_t sp = 0;
        lcd->eraseBox(0, 30, 83, 35);
        tmp = (char*)pgm_read_word(&slist[i].description);
        
        while ((ch = pgm_read_byte(tmp + c))) // Write menu item text //
        {
            sp += lcd->writeCharTiny(sp + c, 30, ch);
            c++;
        }

        lcd->update();
    }

    switch(key)
    {
       case FL_KEY:
       case LEFT_KEY:
           return FN_CANCEL;
        
       case FR_KEY: // save //
            *n = pgm_read_byte(&slist[i].value);
            message(TEXT("Saved"));
            return FN_SAVE;
    }

    return FN_CONTINUE;
}

/******************************************************************
 *
 *   MENU::editDynamic
 *
 *
 ******************************************************************/

char MENU::editDynamic(char key, uint8_t *var, void *ditem, char *name, char first)
{
    char item_name[8];
    dynamicItem_t *item;
    static uint8_t val;
    uint8_t c = 0;
    char ch = 0;
    uint8_t (*npFunc)(uint8_t id);
    uint8_t (*nameFunc)(char name[8], uint8_t id);
    
    item = (dynamicItem_t*)ditem;

    if(first)
    {
        lcd->cls();
        setTitle(name);
        setBar(TEXT("CANCEL"), TEXT("SAVE"));

        // Box //
        lcd->drawBox(2, 12, 71, 24);
        lcd->drawBox(1, 11, 72, 25);

        // Up Arrow //
        lcd->drawLine(74, 17, 82, 17);
        lcd->drawLine(74, 16, 82, 16);
        lcd->drawLine(75, 15, 81, 15);
        lcd->drawLine(76, 14, 80, 14);
        lcd->drawLine(77, 13, 79, 13);
        //   lcd->setPixel(78, 12);

        // Down Arrow //
        lcd->drawLine(74, 19, 82, 19);
        lcd->drawLine(74, 20, 82, 20);
        lcd->drawLine(75, 21, 81, 21);
        lcd->drawLine(76, 22, 80, 22);
        lcd->drawLine(77, 23, 79, 23);
//    lcd->setPixel(78, 24);

        val = *var;
    }


    switch(key)
    {
       case UP_KEY:
           npFunc = (uint8_t (*)(uint8_t))pgm_read_word(&item->nextFunc);
           val = (npFunc)(val);
           ch = 1;
           break;
           
       case DOWN_KEY:
           npFunc = (uint8_t (*)(uint8_t))pgm_read_word(&item->prevFunc);
           val = (npFunc)(val);
           ch = 1;
           break;
    }

    if(ch || first)
    {
        lcd->eraseBox(3, 13, 69, 23);

        nameFunc = (uint8_t (*)(char*, uint8_t))pgm_read_word(&item->nameFunc);
        if((nameFunc)(item_name, val))
        {
          for(c = 0; c < 7; c++) // Write item text //
          {
            if(item_name[c] == 0) break;
            lcd->writeChar(4 + (c + 4) * 6, 15, item_name[c]);
          }        
        }
          
        c = 0;
        char *tmp;
        uint8_t sp = 0;
        lcd->eraseBox(0, 30, 83, 35);
        tmp = (char*)pgm_read_word(&item->description);
        
        while ((ch = pgm_read_byte(tmp + c))) // Write menu item text //
        {
            sp += lcd->writeCharTiny(sp + c, 30, ch);
            c++;
        }

        lcd->update();
    }

    switch(key)
    {
       case FL_KEY:
       case LEFT_KEY:
           return FN_CANCEL;
        
       case FR_KEY: // save //
            *var = val;
            message(TEXT("Saved"));
            return FN_SAVE;
    }

    return FN_CONTINUE;
}

/******************************************************************
 *
 *   MENU::editText
 *
 *
 ******************************************************************/

char MENU::editText(char key, char text[MENU_NAME_LEN], char *name, char first)
{
    static uint8_t i;
    uint8_t c;

    if(first)
    {
        i = 0;
        lcd->cls();
        setTitle(name);
        setBar(TEXT("CANCEL"), TEXT("SAVE"));

        // Box //
        lcd->drawBox(1, 12, 81, 24);
        lcd->drawBox(0, 11, 82, 25);
    }

    switch(key)
    {
       case LEFT_KEY:
           if(i > 0) 
               i--;
           else 
               i = MENU_NAME_LEN - 3;
           first = 1;
           break;
           
       case RIGHT_KEY:
           if(i < MENU_NAME_LEN - 3)
               i++;
           else 
               i = 0;
           first = 1;
           break;
           
       case UP_KEY:
           if((text[i] > 'A' || text[i] > '0') && (text[i] <= '9' || text[i] <= 'Z'))
               text[i]--;
           else if(text[i] == 'A') 
               text[i] = ' ';
           else if(text[i] == ' ') 
               text[i] = '9';
           else 
               text[i] = 'Z';
           
           first = 1;
           break;
           
       case DOWN_KEY:
           if((text[i] < 'Z' || text[i] < '9') && (text[i] >= '0' || text[i] >= 'A')) 
               text[i]++;
           else if(text[i] == 'Z') 
               text[i] = '0';
           else if(text[i] == ' ')
               text[i] = 'A';
           else 
               text[i] = '0';
            
            first = 1;
            break;
    }

    if(first)
    {
        lcd->eraseBox(2, 13, 79, 23);
        
        for(c = 0; c < MENU_NAME_LEN - 2; c++) // Write menu item text //
        {
            if((text[c] < 'A' || text[c] > 'Z') && (text[c] < '0' || text[c] > '9'))
                text[c] = ' ';
            
            lcd->writeChar(3 + c * 7, 15, text[c]);

            if(c == i) 
                lcd->drawHighlight(3 + c * 7, 14, 3 + c * 7 + 6, 15 + 7);
        }

        lcd->update();
    }

    switch(key)
    {
       case FL_KEY:
           return FN_CANCEL;
           
       case FR_KEY:
           return FN_SAVE;
    }

    return FN_CONTINUE;
}

char MENU::alertTask(char key, char first)
{
    if(first)
    {
        uint8_t i = 0, ch, sp = 0, j = 0, y = 0;
        char buf[12];
        lcd->cls();
        // find vertical center
        while(alert_index > 0) // make sure there are alerts available before looping
        {
            ch = pgm_read_byte(alerts[alert_index - 1] + i);
            buf[j] = ch;
            if(j < 12) j++;
            if(ch == ' ' || ch == 0)
            {
              j--;
              buf[j] = 0;
              uint8_t len = lcd->measureStringTiny(buf);
              if(len + 2 + sp >= 84)
              {
                sp = 0;
                y += 6;
              }
              sp += len + 2;
              j = 0;
            }
            i++;
            if(ch == 0) break;
        }
        
        i = 0; sp = 0; j = 0; y = 48 / 2 - y / 2 - 4;

        while(alert_index > 0) // make sure there are alerts available before looping
        {
            ch = pgm_read_byte(alerts[alert_index - 1] + i);
            buf[j] = ch;
            if(j < 12) j++;
            if(ch == ' ' || ch == 0)
            {
              j--;
              buf[j] = 0;
              uint8_t len = lcd->measureStringTiny(buf);
              if(len + 2 + sp >= 84)
              {
                sp = 0;
                y += 6;
              }
              sp += lcd->writeStringTiny(2 + sp, y, buf) + 2;
              j = 0;
            }
            i++;
            if(ch == 0) break;
        }
        setTitle(TEXT("ALERT"));
        setBar(BLANK_STR, TEXT("OK"));
        lcd->update();
    }
    if(key == FR_KEY) return FN_CANCEL;
    return FN_CONTINUE;
}

void MENU::alert(const char *progmem_string)
{
  if(alert_index < MAX_ALERTS)
  {
    if(alert_index == 0) alert_return_state = state;
    alert_index++;
    alerts[alert_index - 1] = (char*)progmem_string;
    state = ST_ALERT_NEW;
  }
}

void MENU::clearAlert(const char *progmem_string)
{
  if(alert_index > 0)
  {
    for(uint8_t i = 0; i < alert_index; i++)
    {
      if(alerts[i] == progmem_string)
      {
        alerts[i] = 0;
      }
    }
  }
}

char MENU::waitingAlert()
{
  if(alert_index > 0) return 1; else return 0;
}

void MENU::blink()
{
  uint8_t bl = lcd->getBacklight();
  lcd->backlight(255);
  hardware_flashlight(1);
  _delay_ms(100);
  lcd->backlight(0);
  hardware_flashlight(0);
  _delay_ms(100);
  lcd->backlight(255);
  hardware_flashlight(1);
  _delay_ms(100);
  lcd->backlight(0);
  hardware_flashlight(0);
  _delay_ms(100);
  lcd->backlight(255);
  hardware_flashlight(1);
  _delay_ms(100);
  lcd->backlight(0);
  hardware_flashlight(0);
  _delay_ms(100);

  lcd->backlight(bl);
}

