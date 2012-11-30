# screenshot.rb
# Grabs screenshots from the Timelapse+
#
# Usage:
#
# ruby ./screenshot.rb "<name>"
#
# It will then create the screenshot <name>.png in the same folder
#
# By Elijah Parker
# mail@timelapseplus.com
#
# For Mac OS X only



require 'serialport'
require 'chunky_png'


class TLP
	def open(port)
		port_str = port
		baud_rate = 9600
		data_bits = 8
		stop_bits = 1
		parity = SerialPort::NONE
		@sp = SerialPort.new(port_str, baud_rate, data_bits, stop_bits, parity)
		@sp.read_timeout=1000
	end

	def id
		@sp.putc('T')
		@sp.getc
	end

	def version
		v = Array.new()
		@sp.putc('v')
		4.times do
			v.push(@sp.getbyte)
		end
		return v
	end

	def screen
		scr = Array.new()
		@sp.putc('S')
		504.times do
			scr.push(@sp.getbyte)
		end
		return scr
	end

	def close
		@sp.close if(@sp)
	end

	def find
		result = false
		[1..10].each do
			list = `ls /dev/tty.usb*`
			sid = "E"
			list.split("\n").each do |dev|
				dev.strip!
				begin
					puts "Trying '" + dev + "'..."
					open(dev)
					tid = id
					result = true if tid == sid
					break
					puts "Invalid ID (" + tid + ") - expected " + sid + ".\n"
					close
				rescue
					puts "Error opening.\n"
					close
					result = false
				end
			end
		end
		return result
	end
end

device = TLP.new
if(device.find)
	v = device.version
	version = 0;
	4.times do |i|
		version += v[i] * (2**(8*i))
	end
	puts version.to_s
end

device.close


