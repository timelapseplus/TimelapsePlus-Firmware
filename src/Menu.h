/*
 *  Menu.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */
 
#define MENU_MESSAGE_DISPLAY_TIME 900

#define MENU_NAME_LEN 13
#define MENU_STACK_SIZE 5
#define MENU_MAX 20

#define ST_CONT 0
#define ST_MENU 1
#define ST_EDIT 2
#define ST_EEPR 3
#define ST_FUNC 4
#define ST_LIST 5
#define ST_TEXT 6
#define ST_SPAWN 7

#define FN_CONTINUE 0
#define FN_CANCEL 1
#define FN_SAVE 2
#define FN_JUMP 3

struct menu_item  // 20 bytes total
{
    char name[MENU_NAME_LEN];  // 13 bytes
    char type;         // 1 byte
    void *function;    // 2 bytes
    void *description; // 2 bytes
    void *condition;   // 2 bytes
};

struct settings_item  // 14 bytes total
{
    char name[MENU_NAME_LEN];  // 13 bytes
    char value;                // 1 byte
    void *description;         // 2 bytes
};

struct menu_stack  // 16 bytes total
{
    char index; // 1 byte
    void *menu;     // 2 bytes
};

class MENU
{
public:
    MENU();
    LCD *lcd;
    Button *button;
    void task();
    void init(menu_item *newmenu);
    void click();
    void spawn(void *function);
    void refresh();
    void up();
    void down();
    void back();
    void highlight(char x1, char y1, char x2, char y2);
    void clearHighlight();
    void select(char menuItem);
    void setTitle(char *title);
    void setBar(char *left, char *right);
    char editNumber(char key, unsigned int *n, char *name, char *unit, char mode, char first);
    char editSelect(char key, char *n, void *settingslist, char *name, char first);
    char editText(char key, char text[MENU_NAME_LEN], char *name, char first);
    char *menuName(char *str);
    void message(char *m);
    void push();

    uint8_t unusedKey;

private:
    void menu_push(void *menu_addr, char selection);
    menu_stack menu_pop();
    char checkScroll();
    uint8_t getIndex(menu_item *cmenu, uint8_t selected);
    uint8_t getSelected(menu_item *cmenu, uint8_t index);

    menu_stack stack[MENU_STACK_SIZE]; // stack for nested menus
    uint8_t stack_counter;
    char menuSelected;
    char menuScroll;
    unsigned char hx1, hy1, hx2, hy2;
    char menuSize;
    menu_item *menu;

    char state;
    char m_refresh;
    unsigned char type;
    unsigned int *var;
    char *bvar;
    settings_item *list;
    char (*func)(char key, char first);
    char (*func_short)(void);
    char (*handlerFunction)(char key, char first);
    char name[MENU_NAME_LEN - 2];
    char desc[MENU_NAME_LEN + 1];
    char *message_text;
    uint32_t message_time;
};


