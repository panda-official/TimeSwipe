#!/usr/bin/env python3
"""
As for 16-bit-address ICs, like, the 24c32, there's no way I could find to use the default smbus library in a way that wouldn't be slow (one byte at a time), as its `read_block_data()` function literally makes the kernel crash. Basically, using the 16-bit interface, you write two address bytes then read the memory (as opposed to 8-bit reads, when you only write one address byte). The 2-byte write is not something the smbus interface devs seem to have planned for, so you need to first write two bytes, then read a block of data explicitly, and the last one is where all falls down. You can read one byte at a time, sure, but it's very slow and I wouldn't even think about it. Also, i2cdump won't really help you there as it doesn't have a mode for reading 16-bit memory chips =(

However, there's smbus2 library which solves that problem by providing an interface to make things like write-word-then-read-block more easy. Here's how you can read and write 16-bit EEPROMs using smbus2:
Installing smbus2 is as easy as `sudo pip install smbus2`.
"""

from smbus2 import SMBus as SMBus2, i2c_msg
from math import ceil
from time import sleep
import regex
import sys, getopt
from struct import *
import crcmod

crcFunction = crcmod.predefined.mkCrcFun('crc-16')

def main(argv):
  outputfile = ""
  inputfile = ""
  i2cAdr = 0x50
  wrongMode = False
  parse = True
  try:
    opts, args = getopt.getopt(argv,"tha:i:o:",["address=","input=","output="])
  except getopt.GetoptError:
    print ('dump.py -o <outputfile>')
    sys.exit(2)
  for opt, arg in opts:
    if opt == '-t':
      wrongMode = True
      print ('wrong firmware mode')
    if opt == '-h':
      print ('dump.py -o <outputfile>')
      sys.exit()
    if opt == '-i':
      inputfile = arg
    elif opt in ("-o", "--output"):
      outputfile = arg
    elif opt in ("-a", "--address"):
      i2cAdr = int(arg, base=16)
  nBytesX = 16
  nBytesY = 256
  if inputfile == "":
    bus2 = SMBus2(0)
    bytes = bytearray(read_from_eeprom_2(bus2, i2cAdr, nBytesX*nBytesY))
  else:
    with open(inputfile,'rb') as ineepfile:
      bytes = ineepfile.read()
  print("eeprom content:")
  buf = "      "
  for i in range(nBytesX): #X Address Header
    buf = buf + " "+ '{:02x}'.format(i)
  buf = buf + "    "
  for i in range(nBytesX): #X String Header
    buf = buf + '{:x}'.format(i)
  print(buf)
  totalWidth = len(buf)
  buf = ""
  for i in range(totalWidth): # Dotted Line
    buf = buf+"-"
  print(buf)
  buf = ""
  nBytesY = int(len(bytes)/nBytesX) +1
  for i in range(nBytesY):
    buf=""
    for j in range(nBytesX):
      index = i*nBytesX+j
      if index <len(bytes):
        buf = buf + " "+'{:02X}'.format(bytes[index])
      else:
        buf = buf + " --"
    buf = buf + " |  "
    for j in range(nBytesX):
      index = i*nBytesX+j
      if index <len(bytes):
        buf = buf +'{:s}'.format(regex.sub(r'\p{C}', '.', chr(bytes[index])))
      else:
        buf = buf + "-"
    print('{:04X}'.format(i*nBytesX)+": "+buf)
  if outputfile != "":
    with open(outputfile,'wb') as eepfile:
      eepfile.write(bytes)
  if parse == True:
    pos = 0
    print('parsed eeprom data:')
    #EEPROM Header
    sig = ""
    version = 0
    numAtoms = 0
    eepLen = 0
    headerData = unpack_from('<ssssBBHL', bytes[pos:])
    pos = pos + 12 #skip header
    hPos = 0
    for i in range(4):
      sig = sig + headerData[hPos].decode("utf-8")
      hPos = hPos + 1
    version = headerData[hPos]
    hPos = hPos + 2 #skip reserved byte
    numAtoms = headerData[hPos]
    hPos = hPos + 1
    eepLen = headerData[hPos]
    print("EEPROM Header:\n sig:\t\t{:s}\n version:\t0x{:02X}\n numAtoms:\t0x{:04X}\n eepLen:\t0x{:08X}".format(sig, version, numAtoms, eepLen))
    #Cycle through all Atoms
    for atomID in range(numAtoms):
      atomHeader = unpack_from('<HHL', bytes[pos:])
      #print(atomHeader)
      pos = pos + 8 #skip header
      atomType = atomHeader[0]
      atomCount = atomHeader[1]
      atomLen = atomHeader[2]
      calcCRC = crcFunction(bytes[pos-8:pos+atomLen-2])
      rawAtom = "{:04X}{:04X}{:08X}".format(atomType,atomCount,atomLen)
      for i in range(atomLen):
        rawAtom = rawAtom + "{:02X}".format(bytes[pos+i])
      buf = ""
      if atomLen-2>0:
        for dataPos in range(atomLen-2):
          if dataPos % 8 == 0:
            buf = buf + "\n\t\t"
          buf = buf + "{:02X}".format(bytes[pos+dataPos])
        pos = pos + dataPos+1 #skip data
      atomCRC = unpack_from('<H', bytes[pos:])
      pos = pos + 2 #skip CRC
      print("Atom\n unparsed:\t{:s}\n type:\t\t0x{:04X}\n count:\t\t0x{:04X}\n dataLen:\t0x{:08X}\n data:{:s}\n CRC:\t\t0x{:04X}\n calcCRC:\t0x{:04X}".format(rawAtom, atomType, atomCount, atomLen, buf, atomCRC[0], calcCRC))


def write_to_eeprom_2(bus, address, data, bs=32, sleep_time=0.01):
    """
    Writes to a 16-bit EEPROM. Only supports starting from 0x0000, for now.
    (to support other start addresses, you'll want to improve the block splitting mechanism)
    Will (or *might*?) raise an IOError with e.errno=121 if the EEPROM is write-protected.
    Default write block size is 32 bytes per write.
    By default, sleeps for 0.01 seconds between writes (otherwise, errors might occur).
    Pass sleep_time=0 to disable that (at your own risk).
    """
    b_l = len(data)
    # Last block may not be complete if data length not divisible by block size
    b_c = int(ceil(b_l/float(bs))) # Block count
    # Actually splitting our data into blocks
    blocks = [data[bs*x:][:bs] for x in range(b_c)]
    for i, block in enumerate(blocks):
        if sleep_time:
            sleep(sleep_time)
        start = i*bs
        hb, lb = start >> 8, start & 0xff
        data = [hb, lb]+block
        write = i2c_msg.write(address, data)
        bus.i2c_rdwr(write)

def read_from_eeprom_2(bus, address, count, bs=32):
    """
    Reads from a 16-bit EEPROM. Only supports starting from 0x0000, for now.
    (to add other start addresses, you'll want to improve the counter we're using)
    Default read block size is 32 bytes per read.
    """
    data = [] # We'll add our read results to here
    # If read count is not divisible by block size,
    # we'll have one partial read at the last read
    full_reads, remainder = divmod(count, bs)
    if remainder: full_reads += 1 # adding that last read if needed
    for i in range(full_reads):
        start = i*bs # next block address
        hb, lb = start >> 8, start & 0xff # into high and low byte
        write = i2c_msg.write(address, [hb, lb])
        # If we're on last cycle and remainder != 0, not doing a full read
        count = remainder if (remainder and i == full_reads-1) else bs
        read = i2c_msg.read(address, count)
        bus.i2c_rdwr(write, read) # combined read&write
        data += list(read)
    return data

if __name__ == "__main__":
   main(sys.argv[1:])
