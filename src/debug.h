/*
 *  debug.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

void debug(char c);
void debug(uint8_t c);
void debug(uint16_t n);
void debug(int16_t n);
void debug(uint32_t n);
void debug(float n);
void debug(char *s);
void debug(const char *s);
void debug_nl(void);
void debug_remote(char *s);

// This can be set per file -- set here to enable globally
#define DEBUG_ENABLED

#ifdef DEBUG_ENABLED
#define DEBUG(x) debug(x)
#define DEBUG_NL debug_nl
#else
#define DEBUG(x)
#define DEBUG_NL()
#endif
