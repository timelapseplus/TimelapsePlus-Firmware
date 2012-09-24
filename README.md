Timelapse--Firmware
===================

Official Firmware for the Timelapse+ Intervalometer


Requirements
------------

avr-gcc  
dfu-programmer  
libusb  


Building
--------

First, connect the Timelapse+ device via USB and boot in DFU mode (hold top two buttons and press the down button, red light will blink).  Then, run the following two commands in the project folder:

make  
make dfu  



