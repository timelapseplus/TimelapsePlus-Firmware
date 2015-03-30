#!/Users/elijah/.rvm/rubies/ruby-1.9.3-p0/bin/ruby
###################################################################
# ©2014 Elijah Parker <mail@timelapseplus.com>    v20140509       #
# This script sets the Timelapse+ MCU fuses to prepare for        #
# flashing bootloader. (must be connected with AVR-ISP, lowspeed) #
###################################################################

if !system("avrdude -p at90usb1287 -P usb -c usbasp -q   -e && avrdude -p at90usb1287 -P usb -c usbasp -q   -U lfuse:w:0xFD:m -U hfuse:w:0xDA:m -U efuse:w:0xFB:m")
	puts "############ Error programming! ############"
else
	puts "############ Success! ############"
end
