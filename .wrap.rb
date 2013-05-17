#!/usr/bin/env ruby

pid = Kernel.fork do
  `#{ARGV.join(" ")}`
  exit
end

trap(:CHLD) do
  print "\n"
  exit
end

loop do
  sleep 10
  begin
    Process.kill(0, pid)
    print '.'
  rescue Errno::ESRCH
    print "\n"
    exit 0
  end
end