#!/usr/bin/ruby

# 0 0 0       1 0.0625 1
# 0 0.0625 0  1 0.125 0.9375
# 0 0.125 0   1 0.1875 0.875

y = 0.0
while (y<1)
  puts "0 #{y} 0  1 #{y+0.0625} #{1.0 - y}"
  
  y += 0.0625;
end
