#define TERM_LINE_HEIGHT 6
#define TERM_WIDTH LCD_WIDTH - 5
#define TERM_HEIGHT LCD_HEIGHT - TERM_LINE_HEIGHT

void termInit();
void termPrintChar(char c);
void termPrintByte(uint8_t c);
void termPrintStr(char *s);
void termClear();
