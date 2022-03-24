#!/usr/bin/env python3
import regex
import sys, getopt
import bitstruct
import argparse
import logging
import json

#globals
inputHexNumber = 0
inputHexBytes = b''

def exit():
    sys.exit()

def serialize(data):
    _data = {}
    for k, v in data.items():
        if isinstance(v, int):
            _data[k] = hex(v)
        else:
            _data[k] = v
    return json.dumps(_data, indent=4)

def parseInputArgs():
  global inputHexBytes
  # Definition of input arguments
  parser = argparse.ArgumentParser()
  parser.add_argument("input", type=str,help="The hexadecimal NVM User Page to parse")
  parser.add_argument("-v", "--verbose", action="store_true", help="verbose output")
  args = parser.parse_args()

  # Parsing of input arguments
  # output verbosity
  if args.verbose:
      logging.basicConfig(level=logging.DEBUG)
      logging.debug("logging verbosity set to debug")
  else:
      logging.basicConfig(level=logging.INFO)
  
  logging.info("input string: %s",args.input)
  
  inputHexBytes = bytes.fromhex(args.input.lstrip("0x"))
  
def parseData():
  global inputHexBytes
  parsedDict = bitstruct.unpack_dict('u1u1u4u4u4u1u1u8u1u3u4u2u4u11u4u2u8u1', ['RES3','WDT_wen','WDT_ewoff','WDT_window','WDT_period','WDT_alwon','WDT_enable','RES2','RAM_eccdis','PSZ','SBLK','RES1', 'NVM_BOOTPROT', 'BOD33_cal', 'BOD33_hyst', 'BOD33_action' ,'BOD33_level', 'BOD33_disable'], inputHexBytes)

  logging.info(serialize(parsedDict))

if __name__ == "__main__":
   parseInputArgs()
   parseData()
   