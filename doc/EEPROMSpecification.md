# EEPROM Specification

**The Specification is based on https://github.com/raspberrypi/hats**

## EEPROM Structure

```
  HEADER  <- EEPROM header (Required)
  ATOM1   <- Vendor info atom (Required)
  ATOM2   <- GPIO map atom (Required)
  ATOM3   <- DT blob atom (Required for compliance with the HAT specification)
  ATOM4   <- Status & Calibration Atom
  ...
  ATOMn
```

## EEPROM Header Structure

```
  Bytes   Field
  4       signature   signature: 0x52, 0x2D, 0x50, 0x69 ("R-Pi" in ASCII)
  1       version     EEPROM data format version (0x00 reserved, 0x01 = first version)
  1       reserved    set to 0
  2       numatoms    total atoms in EEPROM
  4       eeplen      total length in bytes of all eeprom data (including this header)
```

## Atom Structure
```
  Bytes   Field
  2       type        atom type
  2       count       incrementing atom count
  4       dlen        length in bytes of data+CRC
  N       data        N bytes, N = dlen-2
  2       crc16       CRC-16-CCITT of entire atom (type, count, dlen, data)
```

## Atom Types

```
  0x0000 = invalid
  0x0001 = vendor info
  0x0002 = GPIO map
  0x0003 = Linux device tree blob
  0x0004 = manufacturer custom data
  0x0005-0xfffe = reserved for future use
  0xffff = invalid
```

### Vendor info atom data (type=0x0001):


```
  Bytes   Field
  16      uuid        UUID (unique for every single board ever made, based on chip ID)
  2       pid         product ID:                           0x0101 (1st byte - Product Family -> Timeswipe=0x01, Drift=0x02...; 2nd byte - Product ID -> IEPE=0x01, DMS=0x02)
  2       pver        product version:                      board revision according semver MAJOR and MINOR number. (v1.2 = 0x0102)
  1       vslen       vendor string length (bytes):         5
  1       pslen       product string length (bytes):        9
  X       vstr        ASCII vendor string:                  "PANDA"
  Y       pstr        ASCII product string:                 "TimeSwipe"
```

### GPIO map atom data (type=0x0002):

  GPIO map for bank 0 GPIO on 40W B+ header.

  can be found in `tool/ts_hat_config.txt`

### Device Tree atom data (type=0x0003):

Binary data (the name or contents of a `.dtbo` overlay, for board hardware).
Can be found under `tool/ts_hat.dts`
Documentation of how to use it can be found in tool/ts_hat.md

### Status & Calibration Atom (type=0x0004, ATOM4):

Status & Calibration Atom consists of Header containing the timestamp of the calibration procedure and Body containing calibration information.
The board considered calibrated if the Atom exists AND it is valid (by its CRC).
The specification can be found in Calibration Atom Specification.md