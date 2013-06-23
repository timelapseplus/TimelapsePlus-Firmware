/*
 *  Menu_Map.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

const char STR_TIME[]PROGMEM = "h:mm:ss";
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

const char STR_AUX_DISABLED[]PROGMEM = "AUX port disabled";
const char STR_AUX_DOLLY[]PROGMEM = "between frames";
const char STR_AUX_IR[]PROGMEM = "IR signal out";

const char STR_CAMERA_FPS[]PROGMEM = "Frames per Second";
const char STR_DEV_MODE_ON[]PROGMEM = "Shutter lights LED";
const char STR_DEV_MODE_OFF[]PROGMEM = "LED off during use";

const char STR_TIMELAPSE[]PROGMEM = "Basic Timelapse";
const char STR_HDR_TIMELAPSE[]PROGMEM = "HDR Timelapse";
const char STR_HDR_PHOTO[]PROGMEM = "Single HDR Image";
const char STR_BULB_PHOTO[]PROGMEM = "Long Bulb Exposure";
const char STR_BULB_RAMP[]PROGMEM = "Ramp exposure";
const char STR_AUTO_RAMP[]PROGMEM = "Auto Ramp w/meter";
const char STR_HDR_RAMP[]PROGMEM = "Ramp HDR exp";
const char STR_CAMERA_MAKE[]PROGMEM = "Camera make";
const char STR_CAMERA_MAKE_OTHER[]PROGMEM = "Choose if not listed";
const char STR_TIME_SINCE_START[]PROGMEM = "Since start";
const char STR_BRACKET[]PROGMEM = "Stops between exposures";
const char STR_STOPS_DIFFERENCE[]PROGMEM = "Stops difference";
const char STR_STOPS_FROM_START[]PROGMEM = "Stops from start";
const char STR_TV[]PROGMEM = "Shutter Speed";
const char STR_HDR_EXPOSURES[]PROGMEM = "Exposures in set";

const char STR_BT_SLEEP[]PROGMEM = "Connect menu to enable";
const char STR_BT_DISCOVERABLE[]PROGMEM = "Uses more power";
const char STR_HALF_PRESS[]PROGMEM = "Shutter half press";

const char STR_BULB_OFFSET[]PROGMEM = "Offset in ms";
const char STR_DOLLY_PULSE[]PROGMEM = "Pulse len ms";

const char STR_BRAMP_METHOD_KEYFRAME[]PROGMEM = "Keyframe based";
const char STR_BRAMP_METHOD_GUIDED[]PROGMEM = "Manually Guided";
const char STR_BRAMP_METHOD_AUTO[]PROGMEM = "Based on Light";
const char STR_AUTO_BRAMP_INTEGRATION[]PROGMEM = "Light Average";
const char STR_BRAMP_TARGET[]PROGMEM = "Darkest Point";

const char STR_AUTO_RUN_ON[]PROGMEM = "Auto-run TL on pwr";
const char STR_AUTO_RUN_OFF[]PROGMEM = "Normal Operation";

const char STR_MIN_BULB[]PROGMEM = "Minimum Bulb";
const char STR_MAX_ISO[]PROGMEM = "Maximum ISO";
const char STR_MAX_APERTURE[]PROGMEM = "Max Aperture";
const char STR_MIN_APERTURE[]PROGMEM = "Min Aperture";

const menu_item menu_options[]PROGMEM =
{
    { "Stop Timer  ", 'F', (void*)&timerStop, 0, 0, (void*)&timer.running },
    { "Remote Info ", 'F', (void*)&timerStatusRemote, 0, 0, (void*)&showRemoteInfo },
    { "Start Remote", 'F', (void*)&timerRemoteStart, 0, 0, (void*)&showRemoteStart },
    { "Add Keyframe", 'F', (void*)&shutter_addKeyframe, 0, 0, (void*)&modeRampKeyAdd },
    { "Del Keyframe", 'F', (void*)&shutter_removeKeyframe, 0, 0, (void*)&modeRampKeyDel },
//    { "View Details", 'F', (void*)&viewSeconds, 0, 0, 0 },
    { "Load Saved..", 'F', (void*)&shutter_load, 0, 0, (void*)&timerNotRunning },
    { "Save As..   ", 'F', (void*)&shutter_saveAs, 0, 0, 0 },
    { "Save        ", 'F', (void*)&timerSaveCurrent, 0, 0, (void*)&timer.currentId },
    { "Save Default", 'F', (void*)&timerSaveDefault, 0, 0, 0 },
    { "Revert      ", 'F', (void*)&timerRevert, 0, 0, (void*)&timerNotRunning },
    { "\0OPTIONS\0   ", 'F', (void*)&menuBack, (void*)STR_RETURN, 0, (void*)STR_NULL }
};


const settings_item settings_timer_mode[]PROGMEM =
{
    { "Time-lapse  ", MODE_TIMELAPSE, (void*)STR_TIMELAPSE },
    { "HDR T-lapse ", MODE_HDR_TIMELAPSE, (void*)STR_HDR_TIMELAPSE },
    { "HDR Photo   ", MODE_HDR_PHOTO, (void*)STR_HDR_PHOTO },
    { "Bulb Photo  ", MODE_BULB_PHOTO, (void*)STR_BULB_PHOTO },
    { "Bulb-ramp   ", MODE_BULB_RAMP, (void*)STR_BULB_RAMP },
//  { "HDR Ramp    ", MODE_HDR_RAMP, (void *) STR_HDR_RAMP},
    { "\0           ", 0, 0 }
};


const settings_item settings_bramp_method[]PROGMEM =
{
    { "Automatic   ", BRAMP_METHOD_AUTO, (void *) STR_BRAMP_METHOD_AUTO},
    { "Guided      ", BRAMP_METHOD_GUIDED, (void*)STR_BRAMP_METHOD_GUIDED },
    { "Keyframe    ", BRAMP_METHOD_KEYFRAME, (void*)STR_BRAMP_METHOD_KEYFRAME },
    { "\0           ", 0, 0 }
};

const settings_item settings_bramp_target[]PROGMEM =
{
    { "Automatic   ", BRAMP_TARGET_AUTO, (void*)STR_BRAMP_TARGET },
    { "Starlight   ", BRAMP_TARGET_STARS, (void*)STR_BRAMP_TARGET },
    { "Half Moon   ", BRAMP_TARGET_HALFMOON, (void *) STR_BRAMP_TARGET },
    { "Full Moon   ", BRAMP_TARGET_FULLMOON, (void*)STR_BRAMP_TARGET },
    { "\0           ", 0, 0 }
};

const settings_item settings_auto_bramp_integration[]PROGMEM =
{
    { " 1 Minute   ",  1, (void*)STR_AUTO_BRAMP_INTEGRATION },
    { " 4 Minutes  ",  4, (void*)STR_AUTO_BRAMP_INTEGRATION },
    { " 6 Minutes  ",  6, (void*)STR_AUTO_BRAMP_INTEGRATION },
    { " 8 Minutes  ",  8, (void*)STR_AUTO_BRAMP_INTEGRATION },
    { "10 Minutes  ", 10, (void*)STR_AUTO_BRAMP_INTEGRATION },
    { "12 Minutes  ", 12, (void*)STR_AUTO_BRAMP_INTEGRATION },
    { "15 Minutes  ", 15, (void*)STR_AUTO_BRAMP_INTEGRATION },
    { "20 Minutes  ", 20, (void*)STR_AUTO_BRAMP_INTEGRATION },
    { "30 Minutes  ", 30, (void*)STR_AUTO_BRAMP_INTEGRATION },
    { "45 Minutes  ", 45, (void*)STR_AUTO_BRAMP_INTEGRATION },
    { "60 Minutes  ", 60, (void*)STR_AUTO_BRAMP_INTEGRATION },
    { "\0           ", 0, 0 }
};

const settings_item settings_bracket_stops[]PROGMEM =
{
    { "          4 ", 12, (void*)STR_STOPS_DIFFERENCE },
    { "      3 2/3 ", 11, (void*)STR_STOPS_DIFFERENCE },
    { "      3 1/3 ", 10, (void*)STR_STOPS_DIFFERENCE },
    { "          3 ", 9, (void*)STR_STOPS_DIFFERENCE },
    { "      2 2/3 ", 8, (void*)STR_STOPS_DIFFERENCE },
    { "      2 1/3 ", 7, (void*)STR_STOPS_DIFFERENCE },
    { "          2 ", 6, (void*)STR_STOPS_DIFFERENCE },
    { "      1 2/3 ", 5, (void*)STR_STOPS_DIFFERENCE },
    { "      1 1/3 ", 4, (void*)STR_STOPS_DIFFERENCE },
    { "          1 ", 3, (void*)STR_STOPS_DIFFERENCE },
    { "        2/3 ", 2, (void*)STR_STOPS_DIFFERENCE },
    { "        1/3 ", 1, (void*)STR_STOPS_DIFFERENCE },
    { "\0           ", 0, 0 }
};

const settings_item settings_hdr_exposures[]PROGMEM =
{
    { "         13 ", 13, (void*)STR_HDR_EXPOSURES },
    { "         11 ", 11, (void*)STR_HDR_EXPOSURES },
    { "          9 ", 9, (void*)STR_HDR_EXPOSURES },
    { "          7 ", 7, (void*)STR_HDR_EXPOSURES },
    { "          5 ", 5, (void*)STR_HDR_EXPOSURES },
    { "          3 ", 3, (void*)STR_HDR_EXPOSURES },
    { "\0           ", 0, 0 }
};

const dynamicItem_t dyn_tv PROGMEM =
{
    (void*)PTP::shutterUp,
    (void*)PTP::shutterDown,
    (void*)PTP::shutterName,
    (void*)STR_TV
};

const dynamicItem_t dyn_stops PROGMEM =
{
    (void*)stopUp,
    (void*)stopDown,
    (void*)stopName,
    (void*)STR_STOPS_FROM_START
};

const dynamicItem_t dyn_bulb PROGMEM =
{
    (void*)rampTvUp,
    (void*)rampTvDown,
    (void*)PTP::shutterName,
    (void*)STR_TV
};

const dynamicItem_t dyn_min_bulb PROGMEM =
{
    (void*)rampTvUpStatic,
    (void*)rampTvDown,
    (void*)PTP::shutterName,
    (void*)STR_MIN_BULB
};

const dynamicItem_t dyn_max_iso PROGMEM =
{
    (void*)PTP::isoDownStatic,
    (void*)PTP::isoUpStatic,
    (void*)PTP::isoName,
    (void*)STR_MAX_ISO
};

const dynamicItem_t dyn_max_aperture PROGMEM =
{
    (void*)PTP::apertureDownStatic,
    (void*)PTP::apertureUpStatic,
    (void*)PTP::apertureName,
    (void*)STR_MAX_APERTURE
};

const dynamicItem_t dyn_min_aperture PROGMEM =
{
    (void*)PTP::apertureDownStatic,
    (void*)PTP::apertureUpStatic,
    (void*)PTP::apertureName,
    (void*)STR_MIN_APERTURE
};

const dynamicItem_t dyn_hdr_tv PROGMEM =
{
    (void*)hdrTvUp,
    (void*)hdrTvDown,
    (void*)PTP::shutterName,
    (void*)STR_TV
};

const dynamicItem_t dyn_bracket PROGMEM =
{
    (void*)bracketUp,
    (void*)bracketDown,
    (void*)stopName,
    (void*)STR_BRACKET
};

const dynamicItem_t dyn_hdr_exps PROGMEM =
{
    (void*)hdrExpsUp,
    (void*)hdrExpsDown,
    (void*)hdrExpsName,
    (void*)STR_HDR_EXPOSURES
};

const menu_item menu_timelapse[]PROGMEM =
{
    { "Mode       *", 'S', (void*)settings_timer_mode, (void*)&timer.current.Mode, 0, 0 },
    { "Method     *", 'S', (void*)settings_bramp_method, (void*)&timer.current.brampMethod, 0, (void*)&modeRamp },
    { "Delay      T", 'E', (void*)&timer.current.Delay, (void*)STR_TIME, 0, 0 },
    { "Length     T", 'E', (void*)&timer.current.Duration, (void*)STR_TIME, 0, (void*)&modeRamp },
    { "Frames     U", 'E', (void*)&timer.current.Photos, (void*)STR_PHOTOS, 0, (void*)&modeNoRamp },
    { "Intrvl     F", 'E', (void*)&timer.current.Gap, (void*)STR_TIME_TENTHS, 0, (void*)&showGap },
    { "HDR Exp's  +", 'D', (void*)&dyn_hdr_exps,    (void*)&timer.current.Exps, 0, (void*)&modeHDR },
    { "Mid Tv     +", 'D', (void*)&dyn_hdr_tv,            (void*)&timer.current.Exp, 0, (void*)&modeHDR },
    { "Tv         +", 'D', (void*)&dyn_tv,            (void*)&timer.current.Exp, 0, (void*)&modeStandard },
    { "Bracket    +", 'D', (void*)&dyn_bracket,            (void*)&timer.current.Bracket, 0, (void*)&modeHDR },
    { "StartTv    +", 'D', (void*)&dyn_bulb, (void*)&timer.current.BulbStart, 0, (void*)&modeRamp },
    { "Integration ", 'S', (void*)settings_auto_bramp_integration, (void*)&timer.current.Integration, 0, (void*)&brampAuto },
//    { "Night Sky   ", 'S', (void*)settings_bramp_target, (void*)&timer.current.NightSky, 0, (void*)&brampAuto },
    { "-By        T", 'E', (void*)&timer.current.Key[0], (void*)STR_TIME_SINCE_START, 0, (void*)&brampKeyframe },
    { "  Ramp     +", 'D', (void*)&dyn_stops, (void*)&timer.current.Bulb[0], 0, (void*)&brampKeyframe },
    { "-By        T", 'E', (void*)&timer.current.Key[1], (void*)STR_TIME_SINCE_START, 0, (void*)&bulb1 },
    { "  Ramp     +", 'D', (void*)&dyn_stops, (void*)&timer.current.Bulb[1], 0, (void*)&bulb1 },
    { "-By        T", 'E', (void*)&timer.current.Key[2], (void*)STR_TIME_SINCE_START, 0, (void*)&bulb2 },
    { "  Ramp     +", 'D', (void*)&dyn_stops, (void*)&timer.current.Bulb[2], 0, (void*)&bulb2 },
    { "-By        T", 'E', (void*)&timer.current.Key[3], (void*)STR_TIME_SINCE_START, 0, (void*)&bulb3 },
    { "  Ramp     +", 'D', (void*)&dyn_stops, (void*)&timer.current.Bulb[3], 0, (void*)&bulb3 },
    { "-By        T", 'E', (void*)&timer.current.Key[4], (void*)STR_TIME_SINCE_START, 0, (void*)&bulb4 },
    { "  Ramp     +", 'D', (void*)&dyn_stops, (void*)&timer.current.Bulb[4], 0, (void*)&bulb4 },
    { "\0           ", 'F', (void*)&runHandler, (void*)STR_OPTIONS, 0, (void*)STR_RUN }
};

//    { "Shutter    F", 'E', (void*)&timer.current.Exp, (void*)STR_TIME_TENTHS, 0, (void*)&modeStandard },
/*
    { "Bulb 1     F", 'E', (void*)&timer.current.Bulb[0], (void*)STR_TIME_TENTHS, 0, (void*)&modeRamp },
    { "*Key       T", 'E', (void*)&timer.current.Key[0], (void*)STR_TIME_SINCE_START, 0, (void*)&modeRamp },
    { "Bulb 2     F", 'E', (void*)&timer.current.Bulb[1], (void*)STR_TIME_TENTHS, 0, (void*)&modeRamp },
    { "*Key       T", 'E', (void*)&timer.current.Key[1], (void*)STR_TIME_SINCE_START, 0, (void*)&bulb1 },
    { "Bulb 3     F", 'E', (void*)&timer.current.Bulb[2], (void*)STR_TIME_TENTHS, 0, (void*)&bulb1 },
    { "*Key       T", 'E', (void*)&timer.current.Key[2], (void*)STR_TIME_SINCE_START, 0, (void*)&bulb2 },
    { "Bulb 4     F", 'E', (void*)&timer.current.Bulb[3], (void*)STR_TIME_TENTHS, 0, (void*)&bulb2 },
    { "*Key       T", 'E', (void*)&timer.current.Key[3], (void*)STR_TIME_SINCE_START, 0, (void*)&bulb3 },
    { "Bulb 5     F", 'E', (void*)&timer.current.Bulb[4], (void*)STR_TIME_TENTHS, 0, (void*)&bulb3 },
*/
const menu_item menu_trigger[]PROGMEM =
{
    { "Cable Remote", 'F', (void*)cableRelease, 0, 0, (void*)&timerNotRunning },
    { "IR Remote   ", 'F', (void*)IRremote, 0, 0, (void*)&timerNotRunning },
    { "BT Remote   ", 'F', (void*)cableReleaseRemote, 0, 0, (void*)&showRemoteStart },
    { "Lightning   ", 'F', (void*)lightningTrigger, 0, 0, (void*)&timerNotRunning },
    { "Motion      ", 'F', (void*)motionTrigger, 0, 0, (void*)&timerNotRunning },
//    { "Video       ", 'F', (void*)videoRemote, 0, 0, (void*)&camera.supports.video },
    { "Focus Stack ", 'F', (void*)focusStack, 0, 0, (void*)&camera.supports.focus },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_connect[]PROGMEM =
{
    { "Bluetooth   ", 'F', (void*)btConnect, 0, 0, &bt.present },
    { "USB         ", 'F', (void*)usbPlug, 0, 0, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_saved_options[]PROGMEM =
{
    { "Rename      ", 'F', (void*)shutter_rename, 0, 0, 0 },
    { "Delete      ", 'F', (void*)shutter_delete, 0, 0, 0 },
    { "\0OPTIONS\0   ", 'F', (void*)&menuBack, (void*)STR_RETURN, 0, (void*)STR_NULL }
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
    { "Canon       ", CANON, (void*)STR_CAMERA_MAKE },
    { "Minolta     ", MINOLTA, (void*)STR_CAMERA_MAKE },
    { "Nikon       ", NIKON, (void*)STR_CAMERA_MAKE },
    { "Olympus     ", OLYMPUS, (void*)STR_CAMERA_MAKE },
    { "Panasonic   ", PANASONIC, (void*)STR_CAMERA_MAKE },
    { "Pentax      ", PENTAX, (void*)STR_CAMERA_MAKE },
    { "Sony        ", SONY, (void*)STR_CAMERA_MAKE },
    { "Other       ", OTHER, (void*)STR_CAMERA_MAKE_OTHER },
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
    { "10 Minutes  ", 60, (void*)STR_BACKLIGHT_TIME },
    { "5 Minutes   ", 30, (void*)STR_BACKLIGHT_TIME },
    { "2 Minutes   ", 12, (void*)STR_BACKLIGHT_TIME },
    { "1 Minute    ", 6, (void*)STR_BACKLIGHT_TIME },
    { "30 Seconds  ", 3, (void*)STR_BACKLIGHT_TIME },
    { "10 Seconds  ", 1, (void*)STR_BACKLIGHT_TIME },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_power_off_time[]PROGMEM =
{
    { "30 Minutes  ", 180, (void*)STR_POWER_TIME },
    { "10 Minutes  ", 60, (void*)STR_POWER_TIME },
    { "5 Minutes   ", 30, (void*)STR_POWER_TIME },
    { "2 Minutes   ", 12, (void*)STR_POWER_TIME },
    { "1 Minute    ", 6, (void*)STR_POWER_TIME },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_aux_port[]PROGMEM =
{
    { "Disabled    ", AUX_MODE_DISABLED, (void*)STR_AUX_DISABLED },
    { "Dolly Out   ", AUX_MODE_DOLLY, (void*)STR_AUX_DOLLY },
    { "IR Out      ", AUX_MODE_IR, (void*)STR_AUX_IR },
//    { "2nd Camera  ", 60, (void*)STR_AUX_DOLLY },
//    { "PC Sync In  ", 60, (void*)STR_AUX_DOLLY },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_bt_default[]PROGMEM =
{
    { "Power-Off   ", BT_MODE_SLEEP, (void*)STR_BT_SLEEP },
    { "Discoverable", BT_MODE_DISCOVERABLE, (void*)STR_BT_DISCOVERABLE },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_half_press[]PROGMEM =
{
    { "Enabled     ", HALF_PRESS_ENABLED, (void*)STR_HALF_PRESS },
    { "Disabled    ", HALF_PRESS_DISABLED, (void*)STR_HALF_PRESS },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_auto_run[]PROGMEM =
{
    { "Disabled    ", AUTO_RUN_OFF, (void*)STR_AUTO_RUN_OFF },
    { "Run on Pwr  ", AUTO_RUN_ON, (void*)STR_AUTO_RUN_ON },
    { "\0           ", 0, 0 }
};

const char STR_INTERFACE[]PROGMEM = "Camera Interface";

const settings_item menu_settings_interface[]PROGMEM =
{
    { "Auto        ", INTERFACE_AUTO, (void*)STR_INTERFACE },
    { "USB + Cable ", INTERFACE_USB_CABLE, (void*)STR_INTERFACE },
    { "USB Only    ", INTERFACE_USB, (void*)STR_INTERFACE },
    { "Cable Only  ", INTERFACE_IR, (void*)STR_INTERFACE },
    { "IR Only     ", INTERFACE_CABLE, (void*)STR_INTERFACE },
    { "\0           ", 0, 0 }
};

const char STR_BRAMP_ALL[]PROGMEM = "Bulb Aperture ISO";
const char STR_BRAMP_BULB_ISO[]PROGMEM = "Bulb and ISO only";
const char STR_BRAMP_BULB[]PROGMEM = "Only ramp bulb";

const settings_item menu_settings_bramp_mode[]PROGMEM =
{
    { "Bulb,A,ISO  ", BRAMP_MODE_ALL, (void*)STR_BRAMP_ALL },
    { "Bulb, ISO   ", BRAMP_MODE_BULB_ISO, (void*)STR_BRAMP_BULB_ISO },
    { "Bulb only   ", BRAMP_MODE_BULB, (void*)STR_BRAMP_BULB },
    { "\0           ", 0, 0 }
};

const char STR_MODE_SWITCH_ENABLED[]PROGMEM = "Auto change via USB";
const char STR_MODE_SWITCH_DISABLED[]PROGMEM = "Disabled";

const settings_item menu_settings_mode_switch[]PROGMEM =
{
    { "Enabled     ", USB_CHANGE_MODE_ENABLED, (void*)STR_MODE_SWITCH_ENABLED },
    { "Disabled    ", USB_CHANGE_MODE_DISABLED, (void*)STR_MODE_SWITCH_DISABLED },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_flashlight_time[]PROGMEM =
{
    { "10 Minutes  ", 60, (void*)STR_FLASHLIGHT_TIME },
    { "5 Minutes   ", 30, (void*)STR_FLASHLIGHT_TIME },
    { "2 Minutes   ", 12, (void*)STR_FLASHLIGHT_TIME },
    { "1 Minute    ", 6, (void*)STR_FLASHLIGHT_TIME },
    { "30 Seconds  ", 3, (void*)STR_FLASHLIGHT_TIME },
    { "10 Seconds  ", 1, (void*)STR_FLASHLIGHT_TIME },
    { "\0           ", 0, 0 }
};

const settings_item menu_settings_camera_fps[]PROGMEM =
{
    { "12 FPS      ", 8, (void*)STR_CAMERA_FPS },
    { "10 FPS      ", 10, (void*)STR_CAMERA_FPS },
    { "8 FPS       ", 12, (void*)STR_CAMERA_FPS },
    { "7 FPS       ", 14, (void*)STR_CAMERA_FPS },
    { "6 FPS       ", 16, (void*)STR_CAMERA_FPS },
    { "5 FPS       ", 20, (void*)STR_CAMERA_FPS },
    { "4 FPS       ", 25, (void*)STR_CAMERA_FPS },
    { "3 FPS       ", 33, (void*)STR_CAMERA_FPS },
    { "2 FPS       ", 50, (void*)STR_CAMERA_FPS },
    { "1 FPS       ", 100, (void*)STR_CAMERA_FPS },
    { "\0           ", 0, 0 }
};

const char STR_LCD_CONTRAST[]PROGMEM = "LCD Contrast";
const settings_item menu_settings_lcd_contrast[]PROGMEM =
{
    { "Contrast: 1 ", 0xf, (void*)STR_LCD_CONTRAST },
    { "Contrast: 2 ", 0xe, (void*)STR_LCD_CONTRAST },
    { "Contrast: 3 ", 0xd, (void*)STR_LCD_CONTRAST },
    { "Contrast: 4 ", 0xc, (void*)STR_LCD_CONTRAST },
    { "Contrast: 5 ", 0xb, (void*)STR_LCD_CONTRAST },
    { "Contrast: 6 ", 0xa, (void*)STR_LCD_CONTRAST },
    { "Contrast: 7 ", 0x9, (void*)STR_LCD_CONTRAST },
    { "Contrast: 8 ", 0x8, (void*)STR_LCD_CONTRAST },
    { "Contrast: 9 ", 0x7, (void*)STR_LCD_CONTRAST },
    { "Contrast:10 ", 0x6, (void*)STR_LCD_CONTRAST },
    { "Contrast:11 ", 0x5, (void*)STR_LCD_CONTRAST },
    { "Contrast:12 ", 0x4, (void*)STR_LCD_CONTRAST },
    { "Contrast:13 ", 0x3, (void*)STR_LCD_CONTRAST },
    { "Contrast:14 ", 0x2, (void*)STR_LCD_CONTRAST },
    { "Contrast:15 ", 0x1, (void*)STR_LCD_CONTRAST },
    { "\0           ", 0, 0 }
};

const char STR_LCD_COEFFICENT[]PROGMEM = "LCD Temp Coeff.";
const settings_item menu_settings_lcd_coefficent[]PROGMEM =
{
    { "Coefficent3 ", 0x3, (void*)STR_LCD_COEFFICENT },
    { "Coefficent4 ", 0x4, (void*)STR_LCD_COEFFICENT },
    { "Coefficent5 ", 0x5, (void*)STR_LCD_COEFFICENT },
    { "Coefficent6 ", 0x6, (void*)STR_LCD_COEFFICENT },
    { "Coefficent7 ", 0x7, (void*)STR_LCD_COEFFICENT },
    { "\0           ", 0, 0 }
};

const char STR_LCD_BIAS[]PROGMEM = "LCD Bias Mode";
const settings_item menu_settings_lcd_bias[]PROGMEM =
{
    { "Bias Mode 3 ", 0x3, (void*)STR_LCD_BIAS },
    { "Bias Mode 4 ", 0x4, (void*)STR_LCD_BIAS },
    { "\0           ", 0, 0 }
};

const menu_item menu_settings_system[]PROGMEM =
{
    { "System Name ", 'F', (void*)system_name, 0, 0, 0 },
    { "PWR Auto Off", 'S', (void*)menu_settings_power_off_time, (void*)&conf.sysOffTime, (void*)settings_update, 0 },
    { "LED Auto Off", 'S', (void*)menu_settings_flashlight_time, (void*)&conf.flashlightOffTime, (void*)settings_update, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_settings_display[]PROGMEM =
{
    { "BackLt Color", 'S', (void*)menu_settings_lcd_color, (void*)&conf.lcdColor, (void*)settings_update, 0 },
    { "BackLt Time ", 'S', (void*)menu_settings_backlight_time, (void*)&conf.lcdBacklightTime, (void*)settings_update, 0 },
    { "Contrast    ", 'S', (void*)menu_settings_lcd_contrast, (void*)&conf.lcdContrast, (void*)settings_update, 0 },
    { "T Coefficent", 'S', (void*)menu_settings_lcd_coefficent, (void*)&conf.lcdCoefficent, (void*)settings_update, 0 },
    { "Bias Mode   ", 'S', (void*)menu_settings_lcd_bias, (void*)&conf.lcdBias, (void*)settings_update, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_settings_camera[]PROGMEM =
{
    { "Camera Make ", 'S', (void*)menu_settings_camera_make, (void*)&conf.cameraMake, (void*)settings_update, 0 },
    { "Camera FPS  ", 'S', (void*)menu_settings_camera_fps, (void*)&conf.cameraFPS, (void*)settings_update, 0 },
    { "Bulb Mode   ", 'S', (void*)menu_settings_bulb_mode, (void*)&conf.bulbMode, (void*)settings_update, 0 },
    { "Bulb Offset ", 'C', (void*)&conf.bulbOffset, (void*)STR_BULB_OFFSET, (void*)settings_update, 0 },
    { "Bulb Min    ", 'D', (void*)&dyn_min_bulb, (void*)&conf.bulbMin, (void*)settings_update, 0 },
    { "ISO Max     ", 'D', (void*)&dyn_max_iso, (void*)&conf.isoMax, (void*)settings_update, 0 },
    { "Aperture Max", 'D', (void*)&dyn_max_aperture, (void*)&conf.apertureMax, (void*)settings_update, 0 },
    { "Aperture Min", 'D', (void*)&dyn_min_aperture, (void*)&conf.apertureMin, (void*)settings_update, 0 },
    { "Half press  ", 'S', (void*)menu_settings_half_press, (void*)&conf.halfPress, (void*)settings_update, 0 },
    { "Interface   ", 'S', (void*)menu_settings_interface, (void*)&conf.interface, (void*)settings_update, 0 },
    { "Bramp Mode  ", 'S', (void*)menu_settings_bramp_mode, (void*)&conf.brampMode, (void*)settings_update, 0 },
    { "Mode Switch ", 'S', (void*)menu_settings_mode_switch, (void*)&conf.modeSwitch, (void*)settings_update, 0 },
    { "Run on PwrOn", 'S', (void*)menu_settings_auto_run, (void*)&conf.autoRun, (void*)settings_update, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_settings_auxiliary[]PROGMEM =
{
    { "AUX Port    ", 'S', (void*)menu_settings_aux_port, (void*)&conf.auxPort, (void*)settings_update, 0 },
    { "Dolly Pulse ", 'C', (void*)&conf.dollyPulse, (void*)STR_DOLLY_PULSE, (void*)settings_update, &conf.auxPort },
    { "BT Default  ", 'S', (void*)menu_settings_bt_default, (void*)&conf.btMode, (void*)settings_update, &bt.present },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_development[]PROGMEM =
{
    { "Dev Mode LED", 'S', (void*)menu_settings_dev_mode, (void*)&conf.devMode, (void*)settings_update, 0 },
    { "Shutter Test", 'F', (void*)shutterTest, 0, 0, (void*)&timerNotRunning },
    { "Calc BOffset", 'F', (void*)shutterLagTest, 0, 0, (void*)&timerNotRunning },
//    { "4 Hour Light", 'F', (void*)lightTest, 0, 0, (void*)&timerNotRunning },
    { "Sys Status  ", 'F', (void*)sysStatus, 0, 0, 0 },
    { "Battery     ", 'F', (void*)batteryStatus, 0, 0, 0 },
    { "Light Meter ", 'F', (void*)lightMeter, 0, 0, 0 },
//    { "BT Flood    ", 'F', (void*)btFloodTest, 0, 0, 0 },
    { "Reset All   ", 'F', (void*)factoryReset, 0, 0, 0 },
//    { "WDT Reset   ", 'F', (void*)wdtReset, 0, 0, 0 },
    { "DFU Mode    ", 'F', (void*)hardware_bootloader, 0, 0, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_settings[]PROGMEM =
{
    { "System Info ", 'F', (void*)sysInfo, 0, 0, 0 },
    { "System      ", 'M', (void*)menu_settings_system, 0, 0, 0 },
    { "Display     ", 'M', (void*)menu_settings_display, 0, 0, 0 },
    { "Camera      ", 'M', (void*)menu_settings_camera, 0, 0, 0 },
    { "Auxiliary   ", 'M', (void*)menu_settings_auxiliary, 0, 0, 0 },
    { "Development ", 'M', (void*)menu_development, 0, 0, 0 },
    { "\0           ", 'V', 0, 0, 0, 0 }
};

const menu_item menu_main[]PROGMEM =
{
    { "Trigger     ", 'M', (void*)menu_trigger, 0, 0, (void*)&timerNotRunning },
    { "Timelapse   ", 'M', (void*)menu_timelapse, 0, 0, 0 },
    { "Connect     ", 'M', (void*)menu_connect, 0, 0, 0 },
    { "Settings    ", 'M', (void*)menu_settings, 0, 0, 0 },
    { "Power Off   ", 'F', (void*)hardware_off, 0, 0, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

