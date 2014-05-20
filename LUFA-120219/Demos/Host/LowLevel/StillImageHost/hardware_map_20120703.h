/*
 *  hardware_map_20120703.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

#define HARDWARE_VERSION 20120703

/* GENERAL IO */
#define LED_PIN 6, B
#define IR_PIN 7, B
#define TXD_PIN 3, D
#define RXD_PIN 2, D
#define SDA_PIN 1, D
#define SCL_PIN 0, D

/* BUTTON IO */
#define BUTTON_LEFT_PIN 4, D
#define BUTTON_RIGHT_PIN 5, D
#define BUTTON_UP_PIN 7, D
#define BUTTON_DOWN_PIN 6, D
#define BUTTON_FL_PIN 4, E
#define BUTTON_FR_PIN 2, E

/* Light Sense IO */
#define LIGHT_SENSE_PIN 6, E
#define LIGHT_SENSE_ANALOG_PIN 0, F
#define LIGHT_SENSE_CHANNEL 0
#define I2C_LIGHT_INT_PIN 4, A
#define LIGHT_SCALE_1_PIN 0, A
#define LIGHT_SCALE_2_PIN 1, A
#define LIGHT_SCALE_3_PIN 2, A

/* BLUETOOTH IO 8 */
#define BT_FIRMWARE_PIN 7, F
#define BT_ALARM_PIN 5, E
#define BT_RTS_PIN 2, C
#define BT_CTS_PIN 3, F

/* POWER IO */
#define POWER_HOLD_PIN 3, C
#define BATT_SENSE_PIN 2, F
#define BATT_ENABLE_PIN 1, F
#define CHARGE_STATUS_PIN 7, A

/* USB IO */
#define USBID_PIN 3, E
#define UVCONN_PIN 7, E
#define USB_SELECT_PIN 4, F
#define USB_ENABLE_PIN 3, A

/* LCD IO */
#define LCD_RST 5, A
#define LCD_DC  6, A
#define SPI_CS  0, B
#define SPI_MOSI 2, B
#define SPI_SCK 1, B
#define LCD_BL  5, F
#define LCD_BL_DIM  5, B
#define LCD_VCC  1, E

/* SHUTTER IO */
#define SHUTTER_HALF_PIN 6, C
#define SHUTTER_FULL_PIN 7, C
#define SHUTTER_SENSE_PIN 6, F
#define CHECK_CABLE_PIN 4, C
#define AUX_INPUT1_PIN 5, C
#define AUX_INPUT2_PIN 4, B
#define AUX_OUT1_PIN 0, C
#define AUX_OUT2_PIN 1, C
#define AUX_CHECK_CABLE_PIN 0, E

