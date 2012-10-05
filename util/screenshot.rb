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
	image = ChunkyPNG::Image.new(84*2+8, 48*2+8, ChunkyPNG::Color.rgba(230, 230, 230, 255))
	setColor = ChunkyPNG::Color.rgba(64, 64, 64, 255)
	backColor = ChunkyPNG::Color.rgba(220, 220, 220, 255)
	x = 4
	y = 4
	j = 0
	scr = device.screen
	scr.each do |b|
		j += 1
		break if j > 504
		8.times do |i|
			if(b & (1<<i) != 0)
				color = setColor
			else
				color = backColor
			end
			image[x, y + i*2] = color
			image[x, y + i*2+1] = color
			image[x+1, y + i*2] = color
			image[x+1, y + i*2+1] = color
		end
		x += 2
		if(x > 84*2+2)
			y += 16
			x = 4
		end
	end
	name = "screenshot"
	name = ARGV[0] if ARGV[0]
	image.save(name + ".png")
end

device.close


