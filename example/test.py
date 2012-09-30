import usb.core, usb.util
import time

dev = usb.core.find(idVendor=0x59e3, idProduct=0x0a23)
dev.set_configuration(1)


def speedtest():
	ld = -1
	lt = time.time()

	psize = 4096
	ls = 0

	s=''.join([chr(x) for x in range(64)])

	while True:
		##v = int(raw_input())
		##dev.ctrl_transfer(usb.util.CTRL_OUT | usb.util.CTRL_TYPE_VENDOR, 0x23, v, 0, '')
		d = dev.read(0x81, 64, 0, 100)
		print d
		ls += len(d)
		dev.write(0x02, s, 0, 100)
		if ls > 1024*1024:
			t = time.time()
			#print ls, t - lt, round(ls/(t - lt))
			lt = t
			ls = 0

def sendOut(d):
	dev.write(0x02, d, 0, 100)

def vendorGet():
	return dev.ctrl_transfer(0xC0, 0x23, 0, 0, 64)
