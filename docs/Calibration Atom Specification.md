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

### V_In Atom Type
This atom is specific for the TimeSwipe 1.0 board. For each of the 22 different gain settings an error correction line is defined. 
This line is defined by its zero offset and its steepness. y=m*x+b

```
  Bytes   Field
  4       m        steepness (float)
  2       b        zero offset (16bit signed int)
```
This atom is therefore (4+2)*22 = 132 bytes large.

### V_Supply Atom Type
This atom is specific for the TimeSwipe 1.0 board. An error correction line for V_Supply is defined.
This line is defined by its zero offset and its steepness. y=m*x+b

```
  Bytes   Field
  4       m        steepness (float)
  2       b        zero offset (16bit signed int)
```
This atom is therefore (4+2)*1 = 6 bytes large.

### C_In Atom Type
This atom is specific for the TimeSwipe 1.0 board. For each of the 22 different gain settings a error correction line is defined. 
This line is defined by its zero offset and its steepness. y=m*x+b

```
  Bytes   Field
  4       m        steepness (float)
  2       b        zero offset (16bit signed int)
```
This atom is therefore (4+2)*22 = 132 bytes large.
