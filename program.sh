#!/Users/elijah/.rvm/rubies/ruby-1.9.3-p0/bin/ruby
###################################################################
# ©2013 Elijah Parker <mail@timelapseplus.com>    v20130515       #
# This script builds the Timelapse+ firmware and programs the     #
# device, rebooting it to DFU mode if necessary                   #
###################################################################
system("cd /Users/elijah/Desktop/TimelapsePlus/Firmware/TimelapsePlus-Firmware && pwd")
if !system("cd /Users/elijah/Desktop/TimelapsePlus/Firmware/TimelapsePlus-Firmware && make")
	puts "############ Compile Failure! ############"
else
	if !system("cd /Users/elijah/Desktop/TimelapsePlus/Firmware/TimelapsePlus-Firmware && make dfu")
		`cd /Users/elijah/Desktop/TimelapsePlus/Firmware/TimelapsePlus-Firmware && ruby ./util/dfu.rb`
		sleep 3
		if !system("cd /Users/elijah/Desktop/TimelapsePlus/Firmware/TimelapsePlus-Firmware && make dfu")
			puts "############ Error programming device! ############"
		else
			puts "############ Success! ############"
		end
	else
		puts "############ Success! ############"
	end
end
