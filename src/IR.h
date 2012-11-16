/*******************************************
 *
 * Name.......:  cameraIrControl Library
 * Description:  A powerful Library to control easy various cameras via IR. Please check the project page and leave a comment.
 * Author.....:  Sebastian Setz
 * Version....:  1.7
 * Date.......:  2011-12-07
 * Project....:  http://sebastian.setz.name/arduino/my-libraries/multiCameraIrControl
 * Contact....:  http://Sebastian.Setz.name
 * License....:  This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
 *               To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to
 *               Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
 * Keywords...:  arduino, library, camera, ir, control, canon, nikon, olympus, minolta, sony, pentax, interval, timelapse
 * History....:  2010-12-08 V1.0 - release
 *               2010-12-09 V1.1 
 *               2010-12-16 V1.2
 *               2011-01-01 V1.3
 *               2011-01-04 V1.4 - making Sony function work, thank you Walter.
 *               2011-01-25 V1.5 - making Olympus work, thank you Steve Stav.
 *               2011-12-05 V1.6 - adding Olympus zoom, thank you again Steve! Refresh keywords.txt; Arduino 1.0 compatible
 *               2011-12-07 V1.7 - repairing Canon function - thanks to Viktor
 *
 ********************************************/

#ifndef IR_h
#define IR_h

#define IR_PIN 7, B

#define ALL -1
#define CANON 0
#define NIKON 1
#define SONY 2
#define MINOLTA 3
#define OLYMPUS 4
#define PENTAX 5
#define PANASONIC 6

class IR
{
public:
    IR();
    void shutterNow();
    void shutterDelayed();
    void bulbStart();
    void bulbEnd();
    void zoomIn(unsigned int pct);
    void zoomOut(unsigned int pct);

    char make;

private:
    void high40(unsigned int time);
    void high38(unsigned int time);
};

#endif
