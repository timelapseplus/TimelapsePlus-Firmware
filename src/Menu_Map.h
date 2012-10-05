/*
 *  Menu_Map.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

const char STR_TIME[]PROGMEM = "hh:mm:ss";
const char STR_PHOTOS[]PROGMEM = "0 for inf.";
const char STR_TIME_TENTHS[]PROGMEM = "mm:ss.s";
const char STR_EEPROM[]PROGMEM = "EEPROM data";
const char STR_RUN[]PROGMEM = "START";
const char STR_OPTIONS[]PROGMEM = "OPTIONS";
const char STR_RETURN[]PROGMEM = "RETURN";
const char STR_NULL[]PROGMEM = "";

const char STR_BULB_MODE_0[]PROGMEM = "Holds shutter down";
const char STR_BULB_MODE_1[]PROGMEM = "Press on press off";
const char STR_BACKLIGHT_COLOR[]PROGMEM = "Backlight color";

const char STR_BACKLIGHT_TIME[]PROGMEM = "Backlight on time";
const char STR_POWER_TIME[]PROGMEM = "Auto pwr off time";
const char STR_FLASHLIGHT_TIME[]PROGMEM = "LED auto off time";

const menu_item menu_options[]PROGMEM =
{
    { "Stop Timer  ", 'F', (void*)&timerStop, 0, (void*)&timer.running },
    { "Add Keyframe", 'F', (void*)&shutter_addKeyframe, 0, (void*)&modeRampKeyAdd },
    { "Del Keyframe", 'F', (void*)&shutter_removeKeyframe, 0, (void*)&modeRampKeyDel },
    { "View Details", 'F', (void*)&viewSeconds, 0, 0 },
    { "Load Saved..", 'F', (void*)&shutter_load, 0, (void*)&timerNotRunning },
    { "Save As..   ", 'F', (void*)&shutter_saveAs, 0, 0 },
    { "Save        ", 'F', (void*)&timerSaveCurrent, 0, (void*)&timer.current.Name },
    { "Save Default", 'F', (void*)&timerSaveDefault, 0, 0 },
    { "Revert      ", 'F', (void*)&timerRevert, 0, (void*)&timerNotRunning },
    { "\0OPTIONS\0   ", 'F', (void*)&menuBack, (void*)STR_RETURN, (void*)STR_NULL }
};

const char STR_TIMELAPSE[]PROGMEM = "Basic Timelapse";
const char STR_HDR_TIMELAPSE[]PROGMEM = "HDR Timelapse";
const char STR_HDR_PHOTO[]PROGMEM = "Single HDR Image";
const char STR_BULB_RAMP[]PROGMEM = "Ramp exposure";
const char STR_AUTO_RAMP[]PROGMEM = "Auto Ramp w/meter";
const char STR_HDR_RAMP[]PROGMEM = "Ramp HDR exp";
const char STR_CAMERA_MAKE[]PROGMEM = "Camera make";
const char STR_CAMERA_MAKE_OTHER[]PROGMEM = "Choose if not listed";
const char STR_TIME_SINCE_START[]PROGMEM = "Since start";
const char STR_BRACKET[]PROGMEM = "Stops between exposures";
const char STR_CAMERA_FPS[]PROGMEM = "Frames per Second";
const char STR_DEV_MODE_ON[]PROGMEM = "Shutter lights LED";
const char STR_DEV_MODE_OFF[]PROGMEM = "LED off during use";

const settings_item settings_timer_mode[]PROGMEM =
{
    { "Time-lapse  ", MODE_TIMELAPSE, (void*)STR_TIMELAPSE },
    { "HDR T-lapse ", MODE_HDR_TIMELAPSE, (void*)STR_HDR_TIMELAPSE },
    { "HDR Photo   ", MODE_HDR_PHOTO, (void*)STR_HDR_PHOTO },
    { "Bulb-ramp   ", MODE_BULB_RAMP, (void*)STR_BULB_RAMP },
//  {"Auto-ramp   ", MODE_AUTO_RAMP, (void *) STR_AUTO_RAMP},
//  {"HDR Ramp    ", MODE_HDR_RAMP, (void *) STR_HDR_RAMP},
//  {"Auto HDRRamp", MODE_AUTO_HDR_RAMP, (void *) STR_AUTO_HDR_RAMP},
    { "\0           ", 0, 0 }
};

const menu_item menu_timelapse[]PROGMEM =
{
    { "Mode       *", 'S', (void*)settings_timer_mode, (void*)&timer.current.Mode, 0 },
    { "Delay      T", 'E', (void*)&timer.current.Delay, (void*)STR_TIME, 0 },
    { "Frames     U", 'E', (void*)&timer.current.Photos, (void*)STR_PHOTOS, (void*)&modeTimelapse },
    { "Intrvl     F", 'E', (void*)&timer.current.Gap, (void*)STR_TIME_TENTHS, (void*)&showGap },
    { "HDR Exp's  U", 'E', (void*)&timer.current.Exps, (void*)STR_TIME_TENTHS, (void*)&modeHDR },
    { "Shutter    F", 'E', (void*)&timer.current.Exp, (void*)STR_TIME_TENTHS, (void*)&modeHDR },
    { "Shutter    F", 'E', (void*)&timer.current.Exp, (void*)STR_TIME_TENTHS, (void*)&modeStandard },
    { "HDR EV+/-  U", 'E', (void*)&timer.current.Bracket, (void*)STR_BRACKET, (void*)&modeHDR },
    { "Bulb 1     F", 'E', (void*)&timer.current.Bulb[0], (void*)STR_TIME_TENTHS, (void*)&modeRamp },
    { "*Key       T", 'E', (void*)&timer.current.Key[0], (void*)STR_TIME_SINCE_START, (void*)&modeRamp },
    { "Bulb 2     F", 'E', (void*)&timer.current.Bulb[1], (void*)STR_TIME_TENTHS, (void*)&modeRamp },
    { "*Key       T", 'E', (void*)&timer.current.Key[1], (void*)STR_TIME_SINCE_START, (void*)&bulb1 },
    { "Bulb 3     F", 'E', (void*)&timer.current.Bulb[2], (void*)STR_TIME_TENTHS, (void*)&bulb1 },
    { "*Key       T", 'E', (void*)&timer.current.Key[2], (void*)STR_TIME_SINCE_START, (void*)&bulb2 },
    { "Bulb 4     F", 'E', (void*)&timer.current.Bulb[3], (void*)STR_TIME_TENTHS, (void*)&bulb2 },
    { "*Key       T", 'E', (void*)&timer.current.Key[3], (void*)STR_TIME_SINCE_START, (void*)&bulb3 },
    { "Bulb 5     F", 'E', (void*)&timer.current.Bulb[4], (void*)STR_TIME_TENTHS, (void*)&bulb3 },
    { "\0           ", 'F', (void*)&runHandler, (void*)STR_OPTIONS, (void*)STR_RUN }
};

const menu_item menu_trigger[]PROGMEM =
{
    { "Cable Remote", 'F', (void*)cableRelease, 0, (void*)&timerNotRunning },
    { "IR Remote   ", 'F', (void*)IRremote, 0, (void*)&timerNotRunning },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_connect[]PROGMEM =
{
    { "Bluetooth   ", 'F', (void*)notYet, 0, &bt.present },
    { "USB         ", 'F', (void*)usbPlug, 0, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_development[]PROGMEM =
{
    { "Shutter Test", 'F', (void*)shutterTest, 0, (void*)&timerNotRunning },
    { "Shutter Lag ", 'F', (void*)shutterLagTest, 0, (void*)&timerNotRunning },
    { "IR Test     ", 'F', (void*)IRremote, 0, (void*)&timerNotRunning },
    { "4 Hour Light", 'F', (void*)lightTest, 0, (void*)&timerNotRunning },
    { "Sys Status  ", 'F', (void*)sysStatus, 0, 0 },
    { "Battery     ", 'F', (void*)batteryStatus, 0, 0 },
    { "Light Meter ", 'F', (void*)lightMeter, 0, 0 },
    { "Power Off   ", 'F', (void*)hardware_off, 0, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const settings_item menu_settings_bulb_mode[]PROGMEM =
{
    { "Standard    ", 0, (void*)STR_BULB_MODE_0 },
    { "Mom. Toggle ", 1, (void*)STR_BULB_MODE_1 },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_dev_mode[]PROGMEM =
{
    { "Off         ", 0, (void*)STR_DEV_MODE_OFF },
    { "On w/Bulb   ", 1, (void*)STR_DEV_MODE_ON },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_camera_make[]PROGMEM =
{
    { "Other       ", (char)-2, (void*)STR_CAMERA_MAKE_OTHER },
    { "Canon       ", CANON, (void*)STR_CAMERA_MAKE },
    { "Nikon       ", NIKON, (void*)STR_CAMERA_MAKE },
    { "Minolta     ", MINOLTA, (void*)STR_CAMERA_MAKE },
    { "Olympus     ", OLYMPUS, (void*)STR_CAMERA_MAKE },
    { "Pentax      ", PENTAX, (void*)STR_CAMERA_MAKE },
    { "Sony        ", SONY, (void*)STR_CAMERA_MAKE },
    { "Panasonic   ", PANASONIC, (void*)STR_CAMERA_MAKE },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_lcd_color[]PROGMEM =
{
    { "White       ", 0, (void*)STR_BACKLIGHT_COLOR },
    { "Red         ", 1, (void*)STR_BACKLIGHT_COLOR },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_backlight_time[]PROGMEM =
{
    { "10 Seconds  ", 1, (void*)STR_BACKLIGHT_TIME },
    { "30 Seconds  ", 3, (void*)STR_BACKLIGHT_TIME },
    { "1 Minute    ", 6, (void*)STR_BACKLIGHT_TIME },
    { "2 Minutes   ", 12, (void*)STR_BACKLIGHT_TIME },
    { "5 Minutes   ", 30, (void*)STR_BACKLIGHT_TIME },
    { "10 Minutes  ", 60, (void*)STR_BACKLIGHT_TIME },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_power_off_time[]PROGMEM =
{
    { "1 Minute    ", 6, (void*)STR_POWER_TIME },
    { "2 Minutes   ", 12, (void*)STR_POWER_TIME },
    { "5 Minutes   ", 30, (void*)STR_POWER_TIME },
    { "10 Minutes  ", 60, (void*)STR_POWER_TIME },
    { "30 Minutes  ", 180, (void*)STR_POWER_TIME },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_flashlight_time[]PROGMEM =
{
    { "10 Seconds  ", 1, (void*)STR_FLASHLIGHT_TIME },
    { "30 Seconds  ", 3, (void*)STR_FLASHLIGHT_TIME },
    { "1 Minute    ", 6, (void*)STR_FLASHLIGHT_TIME },
    { "2 Minutes   ", 12, (void*)STR_FLASHLIGHT_TIME },
    { "5 Minutes   ", 30, (void*)STR_FLASHLIGHT_TIME },
    { "10 Minutes  ", 60, (void*)STR_FLASHLIGHT_TIME },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_camera_fps[]PROGMEM =
{
    { "1 FPS       ", 101, (void*)STR_CAMERA_FPS },
    { "2 FPS       ", 51, (void*)STR_CAMERA_FPS },
    { "3 FPS       ", 34, (void*)STR_CAMERA_FPS },
    { "4 FPS       ", 26, (void*)STR_CAMERA_FPS },
    { "5 FPS       ", 21, (void*)STR_CAMERA_FPS },
    { "6 FPS       ", 17, (void*)STR_CAMERA_FPS },
    { "7 FPS       ", 15, (void*)STR_CAMERA_FPS },
    { "8 FPS       ", 13, (void*)STR_CAMERA_FPS },
    { "10 FPS      ", 11, (void*)STR_CAMERA_FPS },
    { "12 FPS      ", 9, (void*)STR_CAMERA_FPS },
    { "\0           ", 0, 0 }
};

const menu_item menu_settings[]PROGMEM =
{
    { "System Info ", 'F', (void*)sysInfo, 0, 0 },
    { "LCD Color   ", 'S', (void*)menu_settings_lcd_color, (void*)&conf.lcdColor, (void*)settings_update },
    { "LCD BL Time ", 'S', (void*)menu_settings_backlight_time, (void*)&conf.lcdBacklightTime, (void*)settings_update },
    { "PWR Auto Off", 'S', (void*)menu_settings_power_off_time, (void*)&conf.sysOffTime, (void*)settings_update },
    { "LED Auto Off", 'S', (void*)menu_settings_flashlight_time, (void*)&conf.flashlightOffTime, (void*)settings_update },
    { "Bulb Mode   ", 'S', (void*)menu_settings_bulb_mode, (void*)&conf.bulbMode, (void*)settings_update },
    { "Camera FPS  ", 'S', (void*)menu_settings_camera_fps, (void*)&conf.cameraFPS, (void*)settings_update },
    { "Camera Make ", 'S', (void*)menu_settings_camera_make, (void*)&conf.cameraMake, (void*)settings_update },
    { "Dev Mode LED", 'S', (void*)menu_settings_dev_mode, (void*)&conf.devMode, (void*)settings_update },
    { "Development ", 'M', (void*)menu_development, 0, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_main[]PROGMEM =
{
    { "Trigger     ", 'M', (void*)menu_trigger, 0, (void*)&timerNotRunning },
    { "Timelapse   ", 'M', (void*)menu_timelapse, 0, (void*)&timerQuickNotRunning },
    { "Connect     ", 'M', (void*)menu_connect, 0, 0 },
    { "Settings    ", 'M', (void*)menu_settings, 0, 0 },
    { "Power Off   ", 'F', (void*)hardware_off, 0, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

