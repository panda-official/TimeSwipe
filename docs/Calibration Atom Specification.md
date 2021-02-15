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
  0x0001 = V_In1
  0x0002 = V_In2
  0x0003 = V_In3
  0x0004 = V_In4
  0x0005 = V_supply
  0x0006 = C_In1
  0x0007 = C_In2
  0x0008 = C_In3
  0x0009 = C_In4
  0x000A = Ana_Out
  0x000B-0xfffe ...
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
The definition of how to apply this data needs to be defined.

```
  Bytes   Field
  4       m        slope (float)
  2       b        zero offset (16bit signed int)
```
This atom is therefore (4+2)*22 = 132 bytes large.

### V_Supply Atom Type
This atom is specific for the TimeSwipe 1.0 board. An error correction line for V_Supply is defined.
This line is defined by its zero offset and its slope. y=m*x+b
The unit of the zero Offset value is the DAC Offset digits applied by the ARM Chip. At startup, the ARM Chip will read this value and applies it to the DAC accordingly.
The unit of the slope is a float, which describes a simple y=f(x) translation curve. This data is read by the timeswipe driver on the Pi and is applied on the fly while processing the input data.

```
  Bytes   Field
  4       m        slope (float)
  2       b        zero offset (16bit signed int -32678..32767)
```
This atom is therefore (4+2)*1 = 6 bytes large.

### C_In Atom Type
This atom is specific for the TimeSwipe 1.0 board. For each of the 22 different gain settings a error correction line is defined. 
This line is defined by its zero offset and its slope. y=m*x+b
The unit of the zero Offset value is the DAC Offset digits applied by the ARM Chip. At startup, the ARM Chip will read this value and applies it to the DAC accordingly.
The unit of the slope is a float, which describes a simple y=f(x) translation curve. This data is read by the timeswipe driver on the Pi and is applied on the fly while processing the input data.

```
  Bytes   Field
  4       m        slope (float)
  2       b        zero offset (16bit signed int)
```
This atom is therefore (4+2)*22 = 132 bytes large.

## Calibration JSON Command

The JSON command to write calibration data to the EEPROM is only implemented in the calibration firmware.

Write the array transmitted in "data" into the cAtom 2 (V_supply).
The array will consist of an array of objects (dictionaries) of the data as defined above. e.g. for V_supply, it will be [{m:1.032,b:12000}].
For other cAtoms there are multiple objects in the array in the order of the gain list.
Request/Response              |  Command
----------------------------- | -------------------------------------------------------------------------------------------------------------------------
request message:              |   js<{ "cAtom" : 2, "data" : [{m: 1.23, b: 12000},{...},...]}\n
successive response message:  |       {"cAtom" : 2, "data" : [{m: 1.23, b: 12000},{...},...]}\n
