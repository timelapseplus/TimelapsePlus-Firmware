/*
 *  tldefs.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

// Defines and types for Timelapse+

#define TEXT(thestring)     const_cast<char *>(thestring)               // use this macro for strings that should eventually be localized
#define PTEXT(thestring)   	PSTR(thestring)								// same as above, but for PROGMEM storage
#define BLANK_STR           const_cast<char *>("")
#define STR(thestring)      const_cast<char *>(thestring)               // use this macro for strings that should not be localized (debug code for example)

