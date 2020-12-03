# Calibration Atom Specification
Version 1 - 12/2020

## Calibration Atom Data Structure

```
  HEADER  <- Calibration header (Required)
  C_ATOM1   <- V_In calibration atom
  C_ATOM2   <- V_Supply calibration atom
  C_ATOM3   <- C_In calibration atom
  C_ATOM4   <- Ana_Out calibration atom
  ...
  C_ATOMn
```

## Calibration Atom Header Structure

```
  Bytes   Field
  1       cversion     Calibration data format version (0x00 reserved, 0x01 = first version)
  8.      timestamp.   64bit unix timestamp of calibration date
  2       numcatoms    total c_atoms in Calibration Atom
  4       callen       total length in bytes of all calibration data (including this header)
```

## C_Atom Structure
```
  Bytes   Field
  2       type        c_atom type
  2       count       incrementing atom count
  4       dlen        length in bytes of data
  N       data        N bytes, N = dlen
```

## C_Atom Types

```
  0x0000 = invalid
  0x0001 = V_In
  0x0002 = V_supply
  0x0003 = C_In
  0x0004 = Ana_Out
  0x0005-0xfffe ...
  0xffff = invalid
```

### Gain Settings
The 22 gain settings are defined by their firmware setting.
The Real Setting is the actual gain, that is applied to the input signal.

1st gain stage | 2nd gain stage | Firmware Setting | Setting Real
---   | ---   | ---   | ---
1     | 0.125 | 0.125 | 1
1.375 | 0.125 | 0.172 | 1.375
1     | 0.25  | 0.25  | 2
1.375 | 0.25  | 0.344 | 2.75
1     | 0.5   | 0.5   | 4
1.375 | 0.5   | 0.688 | 5.5
1     | 1     | 1     | 8
1.375 | 1     | 1.375 | 11
1     | 2     | 2     | 16
1.375 | 2     | 2.75  | 22
1     | 4     | 4     | 32
1.375 | 4     | 5.5   | 44
1     | 8     | 8     | 64
1.375 | 8     | 11    | 88
1     | 16    | 16    | 128
1.375 | 16    | 22    | 176
1     | 32    | 32    | 256
1.375 | 32    | 44    | 352
1     | 64    | 64    | 512
1.375 | 64    | 88    | 704
1     | 128   | 128   | 1024
1.375 | 128   | 176   | 1408

### V_In Atom Type
This atom is specific for the TimeSwipe 1.0 board. For each of the 22 different gain settings an error correction line is defined. 
This line is defined by its zero offset and its slope. y=m*x+b

```
  Bytes   Field
  4       m        slope (float)
  2       b        zero offset (16bit signed int)
```
This atom is therefore (4+2)*22 = 132 bytes large.

### V_Supply Atom Type
This atom is specific for the TimeSwipe 1.0 board. An error correction line for V_Supply is defined.
This line is defined by its zero offset and its slope. y=m*x+b

```
  Bytes   Field
  4       m        slope (float)
  2       b        zero offset (16bit signed int -32678..32767)
```
This atom is therefore (4+2)*1 = 6 bytes large.

### C_In Atom Type
This atom is specific for the TimeSwipe 1.0 board. For each of the 22 different gain settings a error correction line is defined. 
This line is defined by its zero offset and its slope. y=m*x+b

```
  Bytes   Field
  4       m        slope (float)
  2       b        zero offset (16bit signed int)
```
This atom is therefore (4+2)*22 = 132 bytes large.

## Calibration JSON Command

The JSON command to write calibration data to the EEPROM is only implemented in the calibration firmware.

tbd...
Request/Response              |  Command
----------------------------- | -------------------------------------------------------------------------------------------------------------------------
request message:              |   js<{ "Gain" : 3, "Bridge" : true,   "DAC1.raw" : 500, "DAC2.raw" : 700, "DAC3.raw" : 900, "DAC4.raw" : 1100 }\n
successive response message:  |       {"Gain" : 3, "Bridge" : true,   "DAC1.raw" : 500, "DAC2.raw" : 700, "DAC3.raw" : 900, "DAC4.raw" : 1100 }\n
