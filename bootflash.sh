#!/Users/elijah/.rvm/rubies/ruby-1.9.3-p0/bin/ruby
###################################################################
# ©2014 Elijah Parker <mail@timelapseplus.com>    v20140509       #
# This script builds the Timelapse+ firmware and flashes the      #
# bootloader (must be connected with AVR-ISP)                     #
###################################################################
system("cd /Users/elijah/Desktop/TimelapsePlus/Firmware/TimelapsePlus-Firmware && pwd")
if !system("cd /Users/elijah/Desktop/TimelapsePlus/Firmware/TimelapsePlus-Firmware && make")
	puts "############ Compile Failure! ############"
else
	if !system("cd /Users/elijah/Desktop/TimelapsePlus/Firmware/TimelapsePlus-Firmware && avrdude -p at90usb1287 -P usb -c usbasp -U flash:w:timelapseplus-bootloader.hex")
		puts "############ Error programming! ############"
	else
		puts "############ Success! ############"
	end
end
