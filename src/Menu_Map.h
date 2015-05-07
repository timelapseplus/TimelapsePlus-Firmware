/*
 *  Menu_Map.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

const char STR_TIME_HOURS[]PROGMEM = "hh:mm";
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
const char STR_AUX_SYNC[]PROGMEM = "PC sync input";
const char STR_AUX_IR[]PROGMEM = "IR signal out";

const char STR_CAM_DEFAULT[]PROGMEM = "Normal operation"; 
const char STR_CAM_DOLLY[]PROGMEM = "Use for motion sync"; 

const char STR_CAMERA_FPS[]PROGMEM = "Frames per Second";
const char STR_DEV_MODE_ON[]PROGMEM = "Shutter lights LED";
const char STR_DEV_MODE_OFF[]PROGMEM = "LED off during use";

const char STR_DEBUG_MODE_ON[]PROGMEM = "Serial debug out";
const char STR_DEBUG_MODE_OFF[]PROGMEM = "Default";

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
const char STR_BT_NMX[]PROGMEM = "Auto connect NMX";
const char STR_HALF_PRESS[]PROGMEM = "Shutter half press";

const char STR_BULB_OFFSET[]PROGMEM = "Offset in ms";
const char STR_DOLLY_PULSE[]PROGMEM = "Pulse len ms";

const char STR_BRAMP_METHOD_KEYFRAME[]PROGMEM = "Keyframe based";
const char STR_BRAMP_METHOD_GUIDED[]PROGMEM = "Manually Guided";
const char STR_BRAMP_METHOD_AUTO[]PROGMEM = "Based on Light";
const char STR_AUTO_BRAMP_INTEGRATION[]PROGMEM = "Light Average";
const char STR_BRAMP_TARGET[]PROGMEM = "Darkest Point";
const char STR_BRAMP_TARGET_CUSTOM[]PROGMEM = "Custom night exp";

const char STR_AUTO_RUN_ON[]PROGMEM = "Auto-run TL on pwr";
const char STR_AUTO_RUN_OFF[]PROGMEM = "Normal Operation";

const char STR_MIN_BULB[]PROGMEM = "Minimum Bulb Time";
const char STR_MAX_ISO[]PROGMEM = "Maximum ISO Ramp";
const char STR_MAX_APERTURE[]PROGMEM = "Max Aperture Ramp";
const char STR_MIN_APERTURE[]PROGMEM = "Min Aperture Ramp";

const char STR_NIGHT_SHUTTER[]PROGMEM = "Night Shutter";
const char STR_NIGHT_ISO[]PROGMEM = "Night ISO";
const char STR_NIGHT_APERTURE[]PROGMEM = "Night Aperture";

const char STR_INTERVAL_MODE_FIXED[]PROGMEM = "Fixed Interval";
const char STR_INTERVAL_MODE_VARIABLE[]PROGMEM = "Variable Interval";
const char STR_INTERVAL_MODE_EXTERNAL[]PROGMEM = "External Trigger";
const char STR_INTERVAL_MODE_KEYFRAME[]PROGMEM = "Keyframe Defined";

const char STR_TUNING[]PROGMEM = "PID Multiplier";
const char STR_THRESHOLD[]PROGMEM = "Dark Value";


const settings_item settings_timer_mode[]PROGMEM =
{
    { "Time-lapse  ", MODE_TIMELAPSE, (void*)STR_TIMELAPSE, 0 },
    { "HDR T-lapse ", MODE_HDR_TIMELAPSE, (void*)STR_HDR_TIMELAPSE, 0 },
    { "HDR Photo   ", MODE_HDR_PHOTO, (void*)STR_HDR_PHOTO, 0 },
    { "Bulb Photo  ", MODE_BULB_PHOTO, (void*)STR_BULB_PHOTO, 0 },
    { "Bulb-ramp   ", MODE_BULB_RAMP, (void*)STR_BULB_RAMP, 1 }
//  { "HDR Ramp    ", MODE_HDR_RAMP, (void *) STR_HDR_RAMP, 1},
};


const settings_item settings_bramp_method[]PROGMEM =
{
    { "Automatic   ", BRAMP_METHOD_AUTO, (void *) STR_BRAMP_METHOD_AUTO, 0 },
    { "Guided      ", BRAMP_METHOD_GUIDED, (void*)STR_BRAMP_METHOD_GUIDED, 0 },
    { "Keyframe    ", BRAMP_METHOD_KEYFRAME, (void*)STR_BRAMP_METHOD_KEYFRAME, 1 }
};

const settings_item settings_bramp_target[]PROGMEM =
{
    { "Automatic   ", BRAMP_TARGET_AUTO, (void*)STR_BRAMP_TARGET, 0 },
    { "Milky Way-6 ", BRAMP_TARGET_OFFSET - 6, (void*)STR_BRAMP_TARGET, 0 },
    { "Dim  Moon-5 ", BRAMP_TARGET_OFFSET - 3, (void*)STR_BRAMP_TARGET, 0 },
    { "Half Moon-4 ", BRAMP_TARGET_OFFSET - 0, (void *) STR_BRAMP_TARGET, 0 },
    { "Full Moon-3 ", BRAMP_TARGET_OFFSET + 3, (void*)STR_BRAMP_TARGET, 0 },
    { "CityLight-2 ", BRAMP_TARGET_OFFSET + 6, (void*)STR_BRAMP_TARGET, 0 },
    { "Bright   -1 ", BRAMP_TARGET_OFFSET + 9, (void*)STR_BRAMP_TARGET, 0 },
    { "Brighter  0 ", BRAMP_TARGET_OFFSET + 12, (void*)STR_BRAMP_TARGET, 0 },
    { "Custom Exp  ", BRAMP_TARGET_CUSTOM, (void*)STR_BRAMP_TARGET_CUSTOM, 1 }
};

const settings_item settings_auto_bramp_integration[]PROGMEM =
{
    { " 1 Minute   ",  1, (void*)STR_AUTO_BRAMP_INTEGRATION, 0 },
    { " 4 Minutes  ",  4, (void*)STR_AUTO_BRAMP_INTEGRATION, 0 },
    { " 6 Minutes  ",  6, (void*)STR_AUTO_BRAMP_INTEGRATION, 0 },
    { " 8 Minutes  ",  8, (void*)STR_AUTO_BRAMP_INTEGRATION, 0 },
    { "10 Minutes  ", 10, (void*)STR_AUTO_BRAMP_INTEGRATION, 0 },
    { "12 Minutes  ", 12, (void*)STR_AUTO_BRAMP_INTEGRATION, 0 },
    { "15 Minutes  ", 15, (void*)STR_AUTO_BRAMP_INTEGRATION, 0 },
    { "20 Minutes  ", 20, (void*)STR_AUTO_BRAMP_INTEGRATION, 0 },
    { "30 Minutes  ", 30, (void*)STR_AUTO_BRAMP_INTEGRATION, 0 },
    { "45 Minutes  ", 45, (void*)STR_AUTO_BRAMP_INTEGRATION, 0 },
    { "60 Minutes  ", 60, (void*)STR_AUTO_BRAMP_INTEGRATION, 1 }
};

const settings_item settings_bracket_stops[]PROGMEM =
{
    { "          4 ", 12, (void*)STR_STOPS_DIFFERENCE, 0 },
    { "      3 2/3 ", 11, (void*)STR_STOPS_DIFFERENCE, 0 },
    { "      3 1/3 ", 10, (void*)STR_STOPS_DIFFERENCE, 0 },
    { "          3 ", 9, (void*)STR_STOPS_DIFFERENCE, 0 },
    { "      2 2/3 ", 8, (void*)STR_STOPS_DIFFERENCE, 0 },
    { "      2 1/3 ", 7, (void*)STR_STOPS_DIFFERENCE, 0 },
    { "          2 ", 6, (void*)STR_STOPS_DIFFERENCE, 0 },
    { "      1 2/3 ", 5, (void*)STR_STOPS_DIFFERENCE, 0 },
    { "      1 1/3 ", 4, (void*)STR_STOPS_DIFFERENCE, 0 },
    { "          1 ", 3, (void*)STR_STOPS_DIFFERENCE, 0 },
    { "        2/3 ", 2, (void*)STR_STOPS_DIFFERENCE, 0 },
    { "        1/3 ", 1, (void*)STR_STOPS_DIFFERENCE, 1 }
};

const settings_item settings_hdr_exposures[]PROGMEM =
{
    { "         13 ", 13, (void*)STR_HDR_EXPOSURES, 0 },
    { "         11 ", 11, (void*)STR_HDR_EXPOSURES, 0 },
    { "          9 ", 9, (void*)STR_HDR_EXPOSURES, 0 },
    { "          7 ", 7, (void*)STR_HDR_EXPOSURES, 0 },
    { "          5 ", 5, (void*)STR_HDR_EXPOSURES, 0 },
    { "          3 ", 3, (void*)STR_HDR_EXPOSURES, 1 }
};

const settings_item settings_interval_mode[]PROGMEM =
{
    { "      Fixed ", INTERVAL_MODE_FIXED, (void*)STR_INTERVAL_MODE_FIXED, 0 },
    { "       Auto ", INTERVAL_MODE_AUTO, (void*)STR_INTERVAL_MODE_VARIABLE, 0 },
    { "   Keyframe ", INTERVAL_MODE_KEYFRAME, (void*)STR_INTERVAL_MODE_KEYFRAME, 0 },
    { "   External ", INTERVAL_MODE_EXTERNAL, (void*)STR_INTERVAL_MODE_EXTERNAL, 1 }
};

const dynamicItem_t dyn_night_iso PROGMEM =
{
    (void*)PTP::isoUp,
    (void*)PTP::isoDown,
    (void*)PTP::isoName,
    (void*)STR_NIGHT_ISO
};

const dynamicItem_t dyn_night_aperture PROGMEM =
{
    (void*)PTP::apertureUp,
    (void*)PTP::apertureDown,
    (void*)PTP::apertureName,
    (void*)STR_NIGHT_APERTURE
};

const dynamicItem_t dyn_night_shutter PROGMEM =
{
    (void*)rampTvUp,
    (void*)rampTvDown,
    (void*)PTP::shutterName,
    (void*)STR_NIGHT_SHUTTER
};

const dynamicItem_t dyn_tv PROGMEM =
{
    (void*)tvUp,
    (void*)tvDown,
    (void*)PTP::shutterName,
    (void*)STR_TV
};

const dynamicItem_t dyn_ramp_ext PROGMEM =
{
    (void*)rampTvUpExtended,
    (void*)rampTvDownExtended,
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

const menu_item menu_timelapse_night_exp[]PROGMEM =
{
    { "Shutter    +", 'D', (void*)&dyn_night_shutter, (void*)&timer.current.nightShutter, 0, 0 },
    { "ISO        +", 'D', (void*)&dyn_night_iso, (void*)&timer.current.nightISO, 0, (void*)&rampISO },
    { "Aperture   +", 'D', (void*)&dyn_night_aperture, (void*)&timer.current.nightAperture, 0, (void*)&rampAperture },
    { "\0           ", 'V', 0, 0, 0 }
};
const menu_item menu_timelapse[]PROGMEM =
{
    { "Mode       *", 'S', (void*)settings_timer_mode, (void*)&timer.current.Mode, 0, 0 },
    { "Method     *", 'S', (void*)settings_bramp_method, (void*)&timer.current.brampMethod, 0, (void*)&modeRamp },
    { "Delay      T", 'E', (void*)&timer.current.Delay, (void*)STR_TIME, 0, 0 },
    { "Length     H", 'E', (void*)&timer.current.Duration, (void*)STR_TIME_HOURS, 0, (void*)&showLength },
    { "Frames     U", 'E', (void*)&timer.current.Photos, (void*)STR_PHOTOS, 0, (void*)&modeNoRamp },
    { "Intrvl Mode ", 'S', (void*)settings_interval_mode, (void*)&timer.current.IntervalMode, 0, (void*)&modeRamp },
    { "Intrvl     F", 'E', (void*)&timer.current.Gap, (void*)STR_TIME_TENTHS, 0, (void*)&showGap },
    { "Int Max    F", 'E', (void*)&timer.current.Gap, (void*)STR_TIME_TENTHS, 0, (void*)&showIntervalMaxMin },
    { "Int Min    F", 'E', (void*)&timer.current.GapMin, (void*)STR_TIME_TENTHS, 0, (void*)&showIntervalMaxMin },
    { "Int Ramp    ", 'K', (void*)&timer.current.kfInterval, 0, (void*)&updateKeyframeGroup, (void*)&showKfInterval },
    { "HDR Exp's  +", 'D', (void*)&dyn_hdr_exps,    (void*)&timer.current.Exps, 0, (void*)&modeHDR },
    { "Mid Tv     +", 'D', (void*)&dyn_hdr_tv,            (void*)&timer.current.Exp, 0, (void*)&modeHDR },
    { "Tv         +", 'D', (void*)&dyn_tv,            (void*)&timer.current.Exp, 0, (void*)&modeStandardExp },
    { "S          +", 'D', (void*)&dyn_tv,            (void*)&timer.current.Exp, 0, (void*)&modeStandardExpNikon },
    { "Bulb       F", 'E', (void*)&timer.current.ArbExp,  (void*)STR_TIME_TENTHS, 0, (void*)&modeStandardExpArb },
    { "Bracket    +", 'D', (void*)&dyn_bracket,            (void*)&timer.current.Bracket, 0, (void*)&modeHDR },
    { "StartTv    +", 'D', (void*)&dyn_bulb, (void*)&timer.current.BulbStart, 0, (void*)&modeRampNormal },
    { "StartTv    +", 'D', (void*)&dyn_ramp_ext, (void*)&timer.current.BulbStart, 0, (void*)&modeRampExtended },
    { "EV Ramp     ", 'K', (void*)&timer.current.kfExposure, 0, (void*)&updateKeyframeGroup, (void*)&brampKeyframe },
    { "Axis1 Ramp  ", 'K', (void*)&timer.current.kfMotor1, 0, (void*)&updateKeyframeGroup, (void*)&motor1.connected },
    { "Axis2 Ramp  ", 'K', (void*)&timer.current.kfMotor2, 0, (void*)&updateKeyframeGroup, (void*)&motor2.connected },
    { "Axis3 Ramp  ", 'K', (void*)&timer.current.kfMotor3, 0, (void*)&updateKeyframeGroup, (void*)&motor3.connected },
    { "Focus Ramp  ", 'K', (void*)&timer.current.kfFocus, 0, (void*)&updateKeyframeGroup, (void*)&showFocus },
    { "Night Target", 'S', (void*)settings_bramp_target, (void*)&timer.current.nightMode, 0, (void*)&brampAuto },
    { "  Night Exp ", 'M', (void*)menu_timelapse_night_exp, 0, 0, (void*)&rampTargetCustom },
    { "\0           ", 'F', (void*)&runHandler, (void*)STR_OPTIONS, 0, (void*)STR_RUN }
};

const menu_item menu_trigger[]PROGMEM =
{
    { "Cable Remote", 'F', (void*)cableRelease, 0, 0, (void*)&timerNotRunning },
    { "IR Remote   ", 'F', (void*)IRremote, 0, 0, (void*)&timerNotRunning },
    { "BT Remote   ", 'F', (void*)cableReleaseRemote, 0, 0, (void*)&showRemoteStart },
    { "Light/Motion", 'F', (void*)lightTrigger, 0, 0, (void*)&timerNotRunning },
    { "Focus Stack ", 'F', (void*)focusStack, 0, 0, (void*)&camera.supports.focus },
    { "Video       ", 'F', (void*)videoRemote, 0, 0, (void*)&camera.supports.video },
    { "Remote Video", 'F', (void*)videoRemoteBT, 0, 0, (void*)&showRemoteStart },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_trigger_running[]PROGMEM =
{
    { "BT Remote   ", 'F', (void*)cableReleaseRemote, 0, 0, (void*)&showRemoteStart },
    { "Remote Video", 'F', (void*)videoRemoteBT, 0, 0, (void*)&showRemoteStart },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_timelapse_options[]PROGMEM =
{
    { "Main Menu   ", 'F', (void*)backToMain, 0, 0, 0 },
    { "Guided Mode ", 'F', (void*)timerToGuided, 0, 0, (void*)&brampNotGuided },
    { "Auto Mode   ", 'F', (void*)timerToAuto, 0, 0, (void*)&brampNotAuto },
    { "Stop T-lapse", 'F', (void*)timerStop, 0, 0, 0 },
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
    { "Standard    ", 0, (void*)STR_BULB_MODE_0, 0 },
    { "Mom. Toggle ", 1, (void*)STR_BULB_MODE_1, 1 }
};

const settings_item menu_settings_dev_mode[]PROGMEM =
{
    { "Off         ", 0, (void*)STR_DEV_MODE_OFF, 0 },
    { "On w/Bulb   ", 1, (void*)STR_DEV_MODE_ON, 1 }
};

const settings_item menu_settings_debug_mode[]PROGMEM =
{
    { "Disabled    ", 0, (void*)STR_DEBUG_MODE_OFF, 0 },
    { "Enabled     ", 1, (void*)STR_DEBUG_MODE_ON, 1 }
};

const settings_item menu_settings_camera_make[]PROGMEM =
{
    { "Canon       ", CANON, (void*)STR_CAMERA_MAKE, 0 },
    { "Minolta     ", MINOLTA, (void*)STR_CAMERA_MAKE, 0 },
    { "Nikon       ", NIKON, (void*)STR_CAMERA_MAKE, 0 },
    { "Olympus     ", OLYMPUS, (void*)STR_CAMERA_MAKE, 0 },
    { "Panasonic   ", PANASONIC, (void*)STR_CAMERA_MAKE, 0 },
    { "Pentax      ", PENTAX, (void*)STR_CAMERA_MAKE, 0 },
    { "Sony        ", SONY, (void*)STR_CAMERA_MAKE, 0 },
    { "Other       ", OTHER, (void*)STR_CAMERA_MAKE_OTHER, 1 }
};

const settings_item menu_settings_lcd_color[]PROGMEM =
{
    { "White       ", 0, (void*)STR_BACKLIGHT_COLOR, 0 },
    { "Red         ", 1, (void*)STR_BACKLIGHT_COLOR, 1 }
};

const settings_item menu_settings_backlight_time[]PROGMEM =
{
    { "10 Minutes  ", 60, (void*)STR_BACKLIGHT_TIME, 0 },
    { "5 Minutes   ", 30, (void*)STR_BACKLIGHT_TIME, 0 },
    { "2 Minutes   ", 12, (void*)STR_BACKLIGHT_TIME, 0 },
    { "1 Minute    ", 6, (void*)STR_BACKLIGHT_TIME, 0 },
    { "30 Seconds  ", 3, (void*)STR_BACKLIGHT_TIME, 0 },
    { "10 Seconds  ", 1, (void*)STR_BACKLIGHT_TIME, 1 }
};

const settings_item menu_settings_power_off_time[]PROGMEM =
{
    { "30 Minutes  ", 180, (void*)STR_POWER_TIME, 0 },
    { "10 Minutes  ", 60, (void*)STR_POWER_TIME, 0 },
    { "5 Minutes   ", 30, (void*)STR_POWER_TIME, 0 },
    { "2 Minutes   ", 12, (void*)STR_POWER_TIME, 0 },
    { "1 Minute    ", 6, (void*)STR_POWER_TIME, 1 }
};

const settings_item menu_settings_aux_port[]PROGMEM =
{
    { "Disabled    ", AUX_MODE_DISABLED, (void*)STR_AUX_DISABLED, 0 },
    { "Motion Sync ", AUX_MODE_DOLLY, (void*)STR_AUX_DOLLY, 0 },
    { "PC Sync In  ", AUX_MODE_SYNC, (void*)STR_AUX_SYNC, 0 },
    { "IR Out      ", AUX_MODE_IR, (void*)STR_AUX_IR, 1 }
//    { "2nd Camera  ", 60, (void*)STR_AUX_DOLLY, 1 },
};

const settings_item menu_settings_camera_port[]PROGMEM =
{
    { "Camera      ", 0, (void*)STR_CAM_DEFAULT, 0 },
    { "Motion Sync ", AUX_CAM_DOLLY, (void*)STR_CAM_DOLLY, 1 }
};

const settings_item menu_settings_bt_default[]PROGMEM =
{
    { "Power-Off   ", BT_MODE_SLEEP, (void*)STR_BT_SLEEP, 0 },
    { "NMX Connect ", BT_MODE_NMX, (void*)STR_BT_NMX, 0 },
    { "Discoverable", BT_MODE_DISCOVERABLE, (void*)STR_BT_DISCOVERABLE, 1 }
};

const settings_item menu_settings_half_press[]PROGMEM =
{
    { "Enabled     ", HALF_PRESS_ENABLED, (void*)STR_HALF_PRESS, 0 },
    { "Disabled    ", HALF_PRESS_DISABLED, (void*)STR_HALF_PRESS, 1 }
};

const settings_item menu_settings_auto_run[]PROGMEM =
{
    { "Disabled    ", AUTO_RUN_OFF, (void*)STR_AUTO_RUN_OFF, 0 },
    { "Run on Pwr  ", AUTO_RUN_ON, (void*)STR_AUTO_RUN_ON, 1 }
};

const char STR_INTERFACE[]PROGMEM = "Camera Interface";

const settings_item menu_settings_interface[]PROGMEM =
{
    { "Auto        ", INTERFACE_AUTO, (void*)STR_INTERFACE, 0 },
    { "USB + Cable ", INTERFACE_USB_CABLE, (void*)STR_INTERFACE, 0 },
    { "USB Only    ", INTERFACE_USB, (void*)STR_INTERFACE, 0 },
    { "Cable Only  ", INTERFACE_CABLE, (void*)STR_INTERFACE, 0 },
    { "IR Only     ", INTERFACE_IR, (void*)STR_INTERFACE, 1 }
};

const char STR_NIKON_USB_CAPTURE[]PROGMEM = "Nikon Prop Code";
const char STR_GENERIC_USB_CAPTURE[]PROGMEM = "Standard PTP Code";

const settings_item menu_settings_nikon_usb_capture[]PROGMEM =
{
    { "Nikon       ", 0, (void*)STR_NIKON_USB_CAPTURE, 0 },
    { "Generic PTP ", 1, (void*)STR_GENERIC_USB_CAPTURE, 1 }
};

const char STR_BRAMP_ALL[]PROGMEM = "Bulb Aperture ISO";
const char STR_BRAMP_BULB_ISO[]PROGMEM = "Bulb and ISO only";
const char STR_BRAMP_BULB[]PROGMEM = "Only ramp bulb";

const settings_item menu_settings_bramp_mode[]PROGMEM =
{
    { "Bulb,A,ISO  ", BRAMP_MODE_ALL, (void*)STR_BRAMP_ALL, 0 },
    { "Bulb, ISO   ", BRAMP_MODE_BULB_ISO, (void*)STR_BRAMP_BULB_ISO, 0 },
    { "Bulb only   ", BRAMP_MODE_BULB, (void*)STR_BRAMP_BULB, 1 }
};

const char STR_BRAMP_EXTENDED_DISABLED[]PROGMEM = "Uses Bulb Only";
const char STR_BRAMP_EXTENDED_ENABLED[]PROGMEM =  "Fast Shutter Steps";

const settings_item menu_settings_bramp_extended[]PROGMEM =
{
    { "Disabled    ", 0, (void*)STR_BRAMP_EXTENDED_DISABLED, 0 },
    { "Enabled     ", 1, (void*)STR_BRAMP_EXTENDED_ENABLED, 1 }
};

const char STR_ARBITRARY_BULB_DISABLED[]PROGMEM = "Set by stops";
const char STR_ARBITRARY_BULB_ENABLED[]PROGMEM = "Arbitrary length";

const settings_item menu_settings_arbitrary_bulb[]PROGMEM =
{
    { "1/3 Stops   ", 0, (void*)STR_ARBITRARY_BULB_DISABLED, 0 },
    { "0.1 Seconds ", 1, (void*)STR_ARBITRARY_BULB_ENABLED, 1 }
};

const char STR_MODE_SWITCH_ENABLED[]PROGMEM = "Auto change via USB";
const char STR_MODE_SWITCH_DISABLED[]PROGMEM = "Disabled";

const settings_item menu_settings_mode_switch[]PROGMEM =
{
    { "Enabled     ", USB_CHANGE_MODE_ENABLED, (void*)STR_MODE_SWITCH_ENABLED, 0 },
    { "Disabled    ", USB_CHANGE_MODE_DISABLED, (void*)STR_MODE_SWITCH_DISABLED, 1 }
};

const char STR_MENU_WRAP_ENABLED[]PROGMEM = "Menu wraps at ends";
const char STR_MENU_WRAP_DISABLED[]PROGMEM = "Menu stops at ends";

const settings_item menu_settings_menu_wrap[]PROGMEM =
{
    { "Enabled     ", 1, (void*)STR_MENU_WRAP_ENABLED, 0 },
    { "Disabled    ", 0, (void*)STR_MENU_WRAP_DISABLED, 1 }
};

const char STR_INTERPOLATION[]PROGMEM = "KF Interpolation";

const settings_item menu_settings_interpolation[]PROGMEM =
{
    { "Spline      ", 0, (void*)STR_INTERPOLATION, 0 },
    { "Linear      ", 1, (void*)STR_INTERPOLATION, 1 }
};

const settings_item menu_settings_flashlight_time[]PROGMEM =
{
    { "10 Minutes  ", 60, (void*)STR_FLASHLIGHT_TIME, 0 },
    { "5 Minutes   ", 30, (void*)STR_FLASHLIGHT_TIME, 0 },
    { "2 Minutes   ", 12, (void*)STR_FLASHLIGHT_TIME, 0 },
    { "1 Minute    ", 6, (void*)STR_FLASHLIGHT_TIME, 0 },
    { "30 Seconds  ", 3, (void*)STR_FLASHLIGHT_TIME, 0 },
    { "10 Seconds  ", 1, (void*)STR_FLASHLIGHT_TIME, 1 }
};

const settings_item menu_settings_camera_fps[]PROGMEM =
{
    { "12 FPS      ", 8, (void*)STR_CAMERA_FPS, 0 },
    { "10 FPS      ", 10, (void*)STR_CAMERA_FPS, 0 },
    { "8 FPS       ", 12, (void*)STR_CAMERA_FPS, 0 },
    { "7 FPS       ", 14, (void*)STR_CAMERA_FPS, 0 },
    { "6 FPS       ", 16, (void*)STR_CAMERA_FPS, 0 },
    { "5 FPS       ", 20, (void*)STR_CAMERA_FPS, 0 },
    { "4 FPS       ", 25, (void*)STR_CAMERA_FPS, 0 },
    { "3 FPS       ", 33, (void*)STR_CAMERA_FPS, 0 },
    { "2 FPS       ", 50, (void*)STR_CAMERA_FPS, 0 },
    { "1 FPS       ", 100, (void*)STR_CAMERA_FPS, 1 }
};

const char STR_LCD_CONTRAST[]PROGMEM = "LCD Contrast";
const settings_item menu_settings_lcd_contrast[]PROGMEM =
{
    { "Contrast: 1 ", 0x1, (void*)STR_LCD_CONTRAST, 0 },
    { "Contrast: 2 ", 0x3, (void*)STR_LCD_CONTRAST, 0 },
    { "Contrast: 3 ", 0x6, (void*)STR_LCD_CONTRAST, 0 },
    { "Contrast: 4 ", 0x8, (void*)STR_LCD_CONTRAST, 0 },
    { "Contrast: 5 ", 0xA, (void*)STR_LCD_CONTRAST, 1 }
};

const char STR_ERROR_ALERT1[]PROGMEM = "Flash LCD only";
const char STR_ERROR_ALERT2[]PROGMEM = "Flash LCD + Light";
const char STR_ERROR_ALERT3[]PROGMEM = "No Flashing Lights";
const settings_item menu_settings_error_alert[]PROGMEM =
{
    { "Backlight   ", 0x0, (void*)STR_ERROR_ALERT1, 0 },
    { "Flashlight  ", 0x1, (void*)STR_ERROR_ALERT2, 0 },
    { "Disabled    ", 0x2, (void*)STR_ERROR_ALERT3, 1 }
};

const char STR_BRAMP_PAD[]PROGMEM = "Intrvl - Bulb";

const menu_item menu_settings_system[]PROGMEM =
{
    { "System Name ", 'F', (void*)system_name, 0, 0, 0 },
    { "Menu Wrap   ", 'S', (void*)menu_settings_menu_wrap, (void*)&conf.menuWrap, (void*)settings_update, 0 },
    { "PWR Auto Off", 'S', (void*)menu_settings_power_off_time, (void*)&conf.sysOffTime, (void*)settings_update, 0 },
    { "LED Auto Off", 'S', (void*)menu_settings_flashlight_time, (void*)&conf.flashlightOffTime, (void*)settings_update, 0 },
    { "Reset All   ", 'F', (void*)factoryReset, 0, 0, (void*)&timerNotRunning },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_settings_display[]PROGMEM =
{
    { "BackLt Color", 'S', (void*)menu_settings_lcd_color, (void*)&conf.lcdColor, (void*)settings_update, 0 },
    { "BackLt Time ", 'S', (void*)menu_settings_backlight_time, (void*)&conf.lcdBacklightTime, (void*)settings_update, 0 },
    { "LCD Contrast", 'S', (void*)menu_settings_lcd_contrast, (void*)&conf.lcdContrast, (void*)settings_update, 0 },
    { "Error Alert ", 'S', (void*)menu_settings_error_alert, (void*)&conf.errorAlert, (void*)settings_update, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const char STR_LVMODE[]PROGMEM = "LiveView Command";
const settings_item menu_settings_canon_lv_mode[]PROGMEM =
{
    { "LiveView DPC", 0, (void*)STR_LVMODE, 0 },
    { "LiveView OC ", 1, (void*)STR_LVMODE, 1 }
};

const menu_item menu_settings_camera[]PROGMEM =
{
    { "Camera Make ", 'S', (void*)menu_settings_camera_make, (void*)&conf.camera.cameraMake, (void*)settings_update, 0 },
    { "Run Autoconf", 'F', (void*)autoConfigureCameraTiming, 0, 0, (void*)&timerNotRunning },
    { "Nikon USB   ", 'S', (void*)menu_settings_nikon_usb_capture, (void*)&conf.camera.nikonUSB, (void*)settings_update, (void*)&cameraMakeNikon },
    { "Canon LV    ", 'S', (void*)menu_settings_canon_lv_mode, (void*)&conf.camera.canonLVOC, (void*)settings_update, (void*)&cameraMakeCanon },
    { "Camera FPS  ", 'S', (void*)menu_settings_camera_fps, (void*)&conf.camera.cameraFPS, (void*)settings_update, 0 },
    { "Bulb Mode   ", 'S', (void*)menu_settings_bulb_mode, (void*)&conf.camera.bulbMode, (void*)settings_update, 0 },
    { "Bulb OffsetB", 'C', (void*)&conf.camera.bulbOffset, (void*)STR_BULB_OFFSET, (void*)settings_update, 0 },
    { "Bulb Min    ", 'D', (void*)&dyn_min_bulb, (void*)&conf.camera.bulbMin, (void*)settings_update, 0 },
    { "Half press  ", 'S', (void*)menu_settings_half_press, (void*)&conf.camera.halfPress, (void*)settings_update, 0 },
    { "Interface   ", 'S', (void*)menu_settings_interface, (void*)&conf.camera.interface, (void*)settings_update, 0 },
    { "Mode Switch ", 'S', (void*)menu_settings_mode_switch, (void*)&conf.camera.modeSwitch, (void*)settings_update, 0 },
    { "Bramp Pad   ", 'C', (void*)&conf.camera.brampGap, (void*)STR_BRAMP_PAD, (void*)settings_update, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_settings_timelapse_tuning[]PROGMEM =
{
    { "Integration ", 'S', (void*)settings_auto_bramp_integration, (void*)&conf.lightIntegrationMinutes, (void*)settings_update, 0 },
    { "Threshold   ", 'C', (void*)&conf.lightThreshold,            (void*)STR_THRESHOLD, (void*)settings_update, 0 },
    { "P Tune     F", 'E', (void*)&conf.pFactor,                   (void*)STR_TUNING,                    (void*)settings_update, 0 },
    { "I Tune     F", 'E', (void*)&conf.iFactor,                   (void*)STR_TUNING,                    (void*)settings_update, 0 },
    { "D Tune     F", 'E', (void*)&conf.dFactor,                   (void*)STR_TUNING,                    (void*)settings_update, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};


const char STR_FOCUS_RAMP[]PROGMEM = "Focus Ramp";

const settings_item settings_focus_enabled[]PROGMEM =
{
    { "Disabled    ", 0, (void*)STR_FOCUS_RAMP, 0 },
    { "Enabled     ", 1, (void*)STR_FOCUS_RAMP, 1 }
};

const char STR_MOTION_HOLD[]PROGMEM = "Power Save";

const settings_item settings_motion_hold[]PROGMEM =
{
    { "Disabled    ", 0, (void*)STR_MOTION_HOLD, 0 },
    { "Enabled     ", 1, (void*)STR_MOTION_HOLD, 1 }
};

const char STR_MOTION_SETUP[]PROGMEM = "Move During Setup";
const settings_item settings_motion_setup[]PROGMEM =
{
    { "Enabled     ", 1, (void*)STR_MOTION_SETUP, 0 },
    { "Disable     ", 0, (void*)STR_MOTION_SETUP, 1 }
};

const char STR_PC_SYNC_MODE1[]PROGMEM = "Possible Flicker";
const char STR_PC_SYNC_MODE2[]PROGMEM = "Fail on Disconnect";

const settings_item menu_settings_pc_sync_mode[]PROGMEM =
{
    { "Auto Detect ", 0, (void*)STR_PC_SYNC_MODE1, 0 },
    { "Required    ", 1, (void*)STR_PC_SYNC_MODE2, 1 }
};

const char STR_KFTIMEUNIT[]PROGMEM = "per button press";

const settings_item settings_keyframeTime[]PROGMEM =
{
    { "1 Pixel     ", 0, (void*)STR_KFTIMEUNIT, 0 },
    { "1 Minute    ", 1, (void*)STR_KFTIMEUNIT, 1 }
};

const char STR_MICROSTEPS[]PROGMEM = "Substep resolution";
const settings_item settings_motion_microsteps[]PROGMEM =
{
    { "Default     ", 0, (void*)STR_MICROSTEPS, 0 },
//    { "Full Steps  ", 1, (void*)STR_MICROSTEPS, 0 },
//    { "Half Steps  ", 2, (void*)STR_MICROSTEPS, 0 },
    { "1/4 Steps   ", 4, (void*)STR_MICROSTEPS, 0 },
    { "1/8 Steps   ", 8, (void*)STR_MICROSTEPS, 0 },
    { "1/16 Steps  ", 16, (void*)STR_MICROSTEPS, 1 }
};

const char STR_MOTION_UNITS[]PROGMEM = "Unit Conversion";
const char STR_MOTION_STEPS[]PROGMEM = "Step Size";
const char STR_MOTION_BACKLASH[]PROGMEM = "Backlash Steps";
const char STR_MOTION_SPEED[]PROGMEM = "Steps per second";

const menu_item menu_settings_timelapse_ramping_nmx1[]PROGMEM =
{
    { "Setup Steps ", 'L', (void*)&conf.motionStep1, (void*)STR_MOTION_STEPS, (void*)settings_update, 0 },
    { "Setup Units ", 'L', (void*)&conf.motionUnit1, (void*)STR_MOTION_UNITS, (void*)settings_update, 0 },
    { "MicroSteps  ", 'S', (void*)settings_motion_microsteps, (void*)&conf.motionMicroSteps1, (void*)settings_update, 0 },
    { "Speed       ", 'C', (void*)&conf.motionSpeed1, (void*)STR_MOTION_SPEED, (void*)settings_update, (void*)&conf.motionMicroSteps1 },
    { "BackLash    ", 'C', (void*)&conf.motionBacklash1, (void*)STR_MOTION_BACKLASH, (void*)settings_update, 0 },
    { "Power Save  ", 'S', (void*)settings_motion_hold, (void*)&conf.motionPowerSave1, (void*)settings_update, 0 },
    { "Setup Move  ", 'S', (void*)settings_motion_setup, (void*)&conf.motionSetupMove1, (void*)settings_update, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_settings_timelapse_ramping_nmx2[]PROGMEM =
{
    { "Setup Steps ", 'L', (void*)&conf.motionStep2, (void*)STR_MOTION_STEPS, (void*)settings_update, 0 },
    { "Setup Units ", 'L', (void*)&conf.motionUnit2, (void*)STR_MOTION_UNITS, (void*)settings_update, 0 },
    { "MicroSteps  ", 'S', (void*)settings_motion_microsteps, (void*)&conf.motionMicroSteps2, (void*)settings_update, 0 },
    { "Speed       ", 'C', (void*)&conf.motionSpeed2, (void*)STR_MOTION_SPEED, (void*)settings_update, (void*)&conf.motionMicroSteps2 },
    { "BackLash    ", 'C', (void*)&conf.motionBacklash2, (void*)STR_MOTION_BACKLASH, (void*)settings_update, 0 },
    { "Power Save  ", 'S', (void*)settings_motion_hold, (void*)&conf.motionPowerSave2, (void*)settings_update, 0 },
    { "Setup Move  ", 'S', (void*)settings_motion_setup, (void*)&conf.motionSetupMove2, (void*)settings_update, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_settings_timelapse_ramping_nmx3[]PROGMEM =
{
    { "Setup Steps ", 'L', (void*)&conf.motionStep3, (void*)STR_MOTION_STEPS, (void*)settings_update, 0 },
    { "Setup Units ", 'L', (void*)&conf.motionUnit3, (void*)STR_MOTION_UNITS, (void*)settings_update, 0 },
    { "MicroSteps  ", 'S', (void*)settings_motion_microsteps, (void*)&conf.motionMicroSteps3, (void*)settings_update, 0 },
    { "Speed       ", 'C', (void*)&conf.motionSpeed3, (void*)STR_MOTION_SPEED, (void*)settings_update, (void*)&conf.motionMicroSteps3 },
    { "BackLash    ", 'C', (void*)&conf.motionBacklash3, (void*)STR_MOTION_BACKLASH, (void*)settings_update, 0 },
    { "Power Save  ", 'S', (void*)settings_motion_hold, (void*)&conf.motionPowerSave3, (void*)settings_update, 0 },
    { "Setup Move  ", 'S', (void*)settings_motion_setup, (void*)&conf.motionSetupMove3, (void*)settings_update, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_settings_timelapse_ramping[]PROGMEM =
{
    { "Interpolate ", 'S', (void*)menu_settings_interpolation, (void*)&conf.linearInterpolation, (void*)settings_update, 0 },
    { "Focus Enable", 'S', (void*)settings_focus_enabled, (void*)&conf.focusEnabled, (void*)settings_update, &conf.focusEnabled },
    { "Focus Steps ", 'C', (void*)&conf.focusStep, (void*)STR_MOTION_STEPS, (void*)settings_update, 0 },
    { "KF Time Unit", 'S', (void*)settings_keyframeTime, (void*)&conf.keyframeTimeByMinute, (void*)settings_update, 0 },
    { "NMX Motor 1 ", 'M', (void*)menu_settings_timelapse_ramping_nmx1, 0, 0, 0 },
    { "NMX Motor 2 ", 'M', (void*)menu_settings_timelapse_ramping_nmx2, 0, 0, 0 },
    { "NMX Motor 3 ", 'M', (void*)menu_settings_timelapse_ramping_nmx3, 0, 0, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_settings_timelapse[]PROGMEM =
{
    { "ISO Max     ", 'D', (void*)&dyn_max_iso, (void*)&conf.isoMax, (void*)settings_update, 0 },
    { "Aperture Max", 'D', (void*)&dyn_max_aperture, (void*)&conf.apertureMax, (void*)settings_update, 0 },
    { "Aperture Min", 'D', (void*)&dyn_min_aperture, (void*)&conf.apertureMin, (void*)settings_update, 0 },
    { "Bramp Mode  ", 'S', (void*)menu_settings_bramp_mode, (void*)&conf.brampMode, (void*)settings_update, 0 },
    { "Ext Bramp   ", 'S', (void*)menu_settings_bramp_extended, (void*)&conf.extendedRamp, (void*)settings_update, 0 },
    { "Bulb Units  ", 'S', (void*)menu_settings_arbitrary_bulb, (void*)&conf.arbitraryBulb, (void*)settings_update, 0 },
    { "Run on PwrOn", 'S', (void*)menu_settings_auto_run, (void*)&conf.autoRun, (void*)settings_update, 0 },
    { "KF Ramping  ", 'M', (void*)menu_settings_timelapse_ramping, 0, 0, 0 },
    { "Bramp Tuning", 'M', (void*)menu_settings_timelapse_tuning, 0, 0, 0 },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_settings_auxiliary[]PROGMEM =
{
    { "AUX Port    ", 'S', (void*)menu_settings_aux_port, (void*)&conf.auxPort, (void*)settings_update, 0 },
    { "Camera Port ", 'S', (void*)menu_settings_camera_port, (void*)&conf.cameraPort, (void*)settings_update, (void*)&pcSyncAux },
    { "PC-Sync Mode", 'S', (void*)menu_settings_pc_sync_mode, (void*)&conf.pcSyncRequired, (void*)settings_update, (void*)&pcSyncAux },
    { "MotionPulse1", 'C', (void*)&conf.dollyPulse, (void*)STR_DOLLY_PULSE, (void*)settings_update, (void*)&dollyAux },
    { "MotionPulse2", 'C', (void*)&conf.dollyPulse2, (void*)STR_DOLLY_PULSE, (void*)settings_update, (void*)&dollyAux },
    { "BT Default  ", 'S', (void*)menu_settings_bt_default, (void*)&conf.btMode, (void*)settings_update, (void*)&bt.present },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_development[]PROGMEM =
{
    { "Dev Mode LED", 'S', (void*)menu_settings_dev_mode, (void*)&conf.devMode, (void*)settings_update, 0 },
//    { "Debug Mode  ", 'S', (void*)menu_settings_debug_mode, (void*)&conf.debugEnabled, (void*)settings_update, 0 },
//    { "Shutter Test", 'F', (void*)shutterTest, 0, 0, (void*)&timerNotRunning },
    { "Light Meter ", 'F', (void*)lightMeter, 0, 0, (void*)&timerNotRunning },
    { "Reset All   ", 'F', (void*)factoryReset, 0, 0, (void*)&timerNotRunning },
    { "DFU Mode    ", 'F', (void*)hardware_bootloader, 0, 0, (void*)&timerNotRunning },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_settings[]PROGMEM =
{
    { "System Info ", 'F', (void*)sysInfo, 0, 0, 0 },
    { "System      ", 'M', (void*)menu_settings_system, 0, 0, 0 },
    { "Display     ", 'M', (void*)menu_settings_display, 0, 0, 0 },
    { "Time-lapse  ", 'M', (void*)menu_settings_timelapse, 0, 0, 0 },
    { "Camera      ", 'M', (void*)menu_settings_camera, 0, 0, 0 },
    { "Auxiliary   ", 'M', (void*)menu_settings_auxiliary, 0, 0, 0 },
    { "Development ", 'M', (void*)menu_development, 0, 0, 0 },
    { "\0           ", 'V', 0, 0, 0, 0 }
};

const menu_item menu_main[]PROGMEM =
{
    { "Trigger     ", 'M', (void*)menu_trigger, 0, 0, (void*)&timerNotRunning },
    { "Timelapse   ", 'M', (void*)menu_timelapse, 0, 0, (void*)&timerNotRunning },
    { "Timelapse   ", 'F', (void*)timerStatus, 0, 0, (void*)&timer.running },
    { "Connect     ", 'M', (void*)menu_connect, 0, 0, 0 },
    { "Settings    ", 'M', (void*)menu_settings, 0, 0, 0 },
    { "Power Off   ", 'F', (void*)hardware_off, 0, 0, (void*)&timerNotRunning },
    { "\0           ", 'V', 0, 0, 0 }
};

const menu_item menu_options[]PROGMEM =
{
    { "Stop Timer  ", 'F', (void*)&timerStop, 0, 0, (void*)&timer.running },
    { "Goto StartKF", 'F', (void*)&gotoStart, 0, 0, (void*)&timerNotRunning },
    { "Flatten KFs ", 'F', (void*)&flattenKeyframes, 0, 0, (void*)&timerNotRunning },
    { "Remote Info ", 'F', (void*)&timerStatusRemote, 0, 0, (void*)&showRemoteInfo },
    { "Start Remote", 'F', (void*)&timerRemoteStart, 0, 0, (void*)&showRemoteStart },
    { "Load Saved..", 'F', (void*)&shutter_load, 0, 0, (void*)&timerNotRunning },
    { "Save As..   ", 'F', (void*)&shutter_saveAs, 0, 0, 0 },
    { "Save        ", 'F', (void*)&timerSaveCurrent, 0, 0, (void*)&timer.currentId },
    { "Save Default", 'F', (void*)&timerSaveDefault, 0, 0, 0 },
    { "Revert      ", 'F', (void*)&timerRevert, 0, 0, (void*)&timerNotRunning },
    { "Settings    ", 'M', (void*)menu_settings_timelapse, 0, 0, 0 },
    { "\0OPTIONS\0   ", 'F', (void*)&menuBack, (void*)STR_RETURN, 0, (void*)STR_NULL }
};

