/*
             LUFA Library
     Copyright (C) Dean Camera, 2012.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2012  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header containing macros for possible PIMA commands. Refer to the PIMA standard
 *  documentation for more information on each PIMA command.
 */

#ifndef _PIMA_CODES_H_

	/* Macros: */

#define PIMA_OPERATION_GETDEVICEINFO         0x1001
#define PIMA_OPERATION_OPENSESSION           0x1002
#define PIMA_OPERATION_CLOSESESSION          0x1003

#define EOS_OC_CAPTURE 0x910F
#define EOS_OC_PROPERTY_SET 0x9110
#define EOS_OC_PROPERTY_GET 0x9127

#define EOS_DPC_ISO 0xD103

#define EOS_DVC_ISO_50 0x40
#define EOS_DVC_ISO_100 0x48
#define EOS_DVC_ISO_125 0x4b
#define EOS_DVC_ISO_160 0x4d
#define EOS_DVC_ISO_200 0x50
#define EOS_DVC_ISO_250 0x53
#define EOS_DVC_ISO_320 0x55
#define EOS_DVC_ISO_400 0x58
#define EOS_DVC_ISO_500 0x5b
#define EOS_DVC_ISO_640 0x5d
#define EOS_DVC_ISO_800 0x60
#define EOS_DVC_ISO_1000 0x63
#define EOS_DVC_ISO_1250 0x65
#define EOS_DVC_ISO_1600 0x68
#define EOS_DVC_ISO_3200 0x70

#endif

