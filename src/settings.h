#define VERSION 20120925


struct settings
{
  char cameraFPS;
  char warnTime;      // 2 bytes
  char mirrorTime;    // 2 bytes
  char sysName[10];     	  // 10 bytes
  char bulbMode;
  char cameraMake;
  char lcdColor;
  char lcdBacklightTime;
  char sysOffTime;
  char flashlightOffTime;
  uint32_t version;
  char devMode;
  char test1;
  char test2;
  char test3;
  char test4;
  char test5;
  char test6;
};

void settings_load(void);
void settings_save(void);
void settings_update(void);
void settings_init(void);

