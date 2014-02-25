#!/usr/bin/python
import usb.core, usb.util
import time
import struct
from zlib import crc32
from intelhex import IntelHex
import sys

VID = 0x59E3
PID = 0xBBBB

REQ_INFO = 0xB0
REQ_ERASE = 0xB1
REQ_START_WRITE  = 0xB2
REQ_CRC_APP = 0xB3
REQ_CRC_BOOT = 0xB4
REQ_RESET = 0xBF

PARTS = {
	'1e9441': 'ATxmega16A4U',
	'1e9541': 'ATxmega32A4U',
	'1e9646': 'ATxmega64A4U',
	'1e9746': 'ATxmega128A4U',
}

CRC32_POLY = 0x0080001BL

def atmel_crc(data):
	#cite: http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=80405&start=0
	addr = 0
	d = 0
	ha = 0
	hb = 0
	crc = 0
	
	while addr < len(data):
		ha = crc << 1
		ha &= 0x00FFFFFE
		hb = crc & (1 << 23)
		if hb > 0:
			hb = 0x00FFFFFF
		
		d = ord(data[addr]) | (ord(data[addr+1])<<8)
		crc = (ha ^ d) ^ (hb & CRC32_POLY)
		crc &= 0x00FFFFFF
		addr+=2
	return int(crc)
	
def lookup_part(s):
	return PARTS.get(s[0:6].lower(), 'Unknown')

BKSP = chr(0x08)

class Bootloader(object):
	def __init__(self, vid=VID, pid=PID):
		self.dev = usb.core.find(idVendor=vid, idProduct=pid)
		if not self.dev:
			raise IOError("can't find xmega in bootloader mode")
		self.dev.set_configuration()
		self.read_info()
		print "Bootloader ID %s, version %i"%(self.magic, self.version)
		print "Part ID: %s = %s"%(self.part, lookup_part(self.part))
		print "Flash size: %i (%i-byte pages)"%(self.memsize+1, self.pagesize)
		print "Hardware: %s Version: %s"%(self.hw_product, self.hw_version)

	def read_info(self):
		data = self.dev.ctrl_transfer(0x40|0x80, REQ_INFO, 0, 0, 64)
		magic, self.version, part, self.pagesize, self.memsize, jumpaddr, hw_prod, hw_ver = \
			    struct.unpack("<4s B 4s H I I 16s 16s", data[0:51])
		self.magic = magic.encode('hex')
		self.part = part.encode('hex')
		self.hw_product = hw_prod
		self.hw_version = hw_ver
	
	def app_crc(self):
		data = self.dev.ctrl_transfer(0x40|0x80, REQ_CRC_APP, 0, 0, 4)
		return struct.unpack("<I", data)[0]
	
	def boot_crc(self):
		data = self.dev.ctrl_transfer(0x40|0x80, REQ_CRC_BOOT, 0, 0, 4)
		return struct.unpack("<I", data)[0]
	
	def erase(self):
		self.dev.ctrl_transfer(0x40|0x80, REQ_ERASE, 0, 0, 0)
	
	def reset(self):
		self.dev.ctrl_transfer(0x40|0x80, REQ_RESET, 0, 0, 0)
	
	def program(self, ih):
		self.dev.ctrl_transfer(0x40|0x80, REQ_START_WRITE, 0, 0, 0)
		
		maxaddr = ih.maxaddr()
		if maxaddr > self.memsize:
			raise IOError("Input file size (%s) is too large to fit in flash (%s)"%(maxaddr, self.memsize))

		maxaddr = maxaddr + (self.pagesize - (maxaddr)%self.pagesize - 1) #round up to nearest page
		data = ih.tobinstr(start=0, end=maxaddr, pad=0xff)

		sys.stdout.write('  0%')
		sys.stdout.flush()
		
		i = 0
		tsize = 1024
		while i<len(data):
			self.dev.write(1, data[i:i+tsize])
			i+=tsize
			
			percent = 100*i/len(data)
			sys.stdout.write(BKSP*4+"% 3i%%"%(min(100, round(percent))))

			sys.stdout.flush()
			
		sys.stdout.write('\n')
		sys.stdout.flush()
		
	def write_hex_file(self, fname):
		print "Loading input file", fname
		ih = IntelHex(fname)

		input_crc = atmel_crc(ih.tobinstr(start=0, end=self.memsize, pad=0xff))
		print "Size=%s; CRC=%s"%(ih.maxaddr(), hex(input_crc))
		
		print "Erasing...",
		self.erase()
		print "done"
		
		print "Flashing...",
		self.program(ih)
		
		dev_crc = self.app_crc()
		print "Checked CRC is", hex(dev_crc)
		
		if input_crc == dev_crc:
			print "CRC matches"
			print "Resetting"
			self.reset()
		else:
			print "CRC DOES NOT MATCH"

	def handle_args(self, args):
		if len(sys.argv)!=2:
			print "Usage: flash.py <file.hex>|reset|crc"
			sys.exit(1)
		elif sys.argv[1] == 'reset':
			print "Resetting"
			self.reset()
		elif sys.argv[1] == 'crc':
			print "App CRC:", hex(self.app_crc())
			print "Boot CRC:", hex(self.boot_crc())
		else:
			self.write_hex_file(sys.argv[1])

def enterBootloader(vid, pid):
	device = usb.core.find(idVendor=vid, idProduct=pid)
	if usb.core.find(idVendor=VID, idProduct=PID):
		pass	
	elif not device:
		print "Device not found in app mode"
	else:
		device.ctrl_transfer(0x40|0x80, 0xBB, 0, 0, 1)
		time.sleep(1.5)

if __name__ == "__main__":
	if len(sys.argv) == 4 and sys.argv[1] == 'enter':
		vid = int(sys.argv[2], 16)
		pid = int(sys.argv[3], 16)
		enterBootloader(vid, pid)
	else:
		b = Bootloader()
		b.handle_args(sys.argv)
