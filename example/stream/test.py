import usb.core, usb.util
import time

dev = usb.core.find(idVendor=0x9999, idProduct=0xffff)

d1 = range(1, 64*2+1)
print len(d1)

d1[55] = 0
d1[-2] = 0

"""
for i in range(100):
	dev.write(0x02, d1, 0, 100)
	for i in range(3):
		d = dev.read(0x81, 64, 0, 100)
		print len(d), d
"""

while 1:
	d = dev.read(0x81, 512, 0, 30000)
	print len(d), d

#print d1 == list(d2), d2
