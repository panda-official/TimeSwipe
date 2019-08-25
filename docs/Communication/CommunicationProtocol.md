# Data Model

The board's internal data is represented as access points or variables.
Each variable holds an only single value that can be written(set) to the variable or read out (get) if appropriate access rights are set (r/w)
The value is either primitive type (bool, int, float, etc) or an object.

Each variable(access point) has its own unique name that is written in the domain name form for convenience. Where the elder domain name is on the left
For example: "ADC1.raw" means raw value of ADC channel 1.
This way access points form a hierarchical data model.

Lets see possible access points types:

Type DAC: This access point type is used to control board's DACs and consists of two domain names.

Root domain:        Holds a DAC setpoint in floating-point format (real value, -10:+10 Volts, <float>, r/w)
Sub domain .raw:    Holds a DAC setpoint in a raw binary format (integer value 0:4095 discrets, <int>, r/w)

examples:
"DACA"        an access point for board's DACA setpoint in floating-point format
"DACA.raw"    an access point for board's DACA setpoint in raw binary format

Here is a list of all possible DAC's access points:

DACA <br />
DACA.raw <br />
DACB <br />
DACB.raw <br />
DACC <br />
DACC.raw <br />
DACD <br />
DACD.raw <br />
DAC1.raw <br />
DAC2.raw <br />



Type ADC: This access point type is used to control board's ADCs and consists of two domain names

Root domain:        Can be used only with sub-domain
Sub domain .raw:    Holds an ADC measured value in a raw binary format (integer value 0:4095 discrets, <int>, r)

examples:
"ADC1.raw"   an access point for an actual measured value of board's ADC1 in raw binary format (0-4095, discrets)

Here is a list of all possible ADC's access points:

ADC1.raw <br />
ADC2.raw <br />
ADC3.raw <br />
ADC4.raw <br />



Type LED: This access point type is used to control board's LEDs 

Root domain:        Holds a LED ON/OFF state (boolen, false:true, <bool>, w)
Sub domain .blink   Holds a LED blinking mode (ON or OFF) (boolen, false:true, <bool>, w)
Sub domain .col     Holds a LED color value (integer RGB value 0:0xffffff, <unsigned int>, w)

Here is a list of all possible LED's access points:

LED1 <br />
LED1.blink <br />
LED1.col <br />
LED2 <br />
LED2.blink <br />
LED2.col <br />
LED3 <br />
LED3.blink <br />
LED3.col <br />
LED4 <br />
LED4.blink <br />
LED4.col <br />


The following access points has only one root domain name:

Gain                Holds a gain value (integer value 1:4, <int>, r/w) <br />
Bridge              Holds a bridge switch state (ON or OFF) (boolen, false:true, <bool>, r/w) <br />
Record              Writing "true" to this variable initiates/restarts a record process (boolean, false:true, <bool>, r/w) <br />
Zero                Start/stop zero calibration process (boolean, false:true, <bool>, w) <br />
Zero.errtol         Holds a zero calibration process error tolerance value (integer, <int> r/w) <br />
EnableADmes         Holds an ADC enabled state (ON or OFF) (boolean, false:true, <bool>, r/w) <br />
DACsw               Holds a DACs mode switch state (0 - using external DACs(A-C) only or 1 -using DACA, DACC, DAC0, DAC1) (integer value, 0:1, <int>, r/w) <br />


JSON controlled access points:

js                  Writing to this variable a JSON object leads to write operation on multiple variables pointed in this object
                    The result of the operation will be returned as a JSON object (see the protocol description below)
                    Reading from this variable a JSON object leads to readout values from all of the pointed variables as a JSON object
                    (JSON object, r/w)
                    
je                  Holds latest events description in a form of a JSON object (JSON object, r)

The structure of the JSON can be arbitrary but must follow a semantic rule: {"variable name" : value}. When reading information from a variable value is ignored and should be set to zero.

examples: 

{
  {"DACA", 0.1},
  {"DACB", -1.2},
  {"DACC", 0.5},
  {"DACD", 1.6}
}
                - a JSON object for setting a group of the 4 DACS
                

{
  {"DACA", 0},
  {"DACB", 0},
  {"DACC", 0},
  {"DACD", 0}
}
                - a JSON object for read back values of a group of the 4 DACS


# Communication protocols

There are a number of protocols can be used for managing data of access points.
The simple ANSI text protocol is implemented by default.
The protocol works according to a master-slave communication model.

A request from a master consists of: access point(variable) name, access operator ('>' means read variable, '<' means write one), [value(object)], new line character (/n)
A responce from a slave is a read back value(object) in a case of success or an error message started with '!' character

Common communication errors for all access points are listed below:

!Line_err!              - communication bus error <br />
!!Timeout_err!          - the board is not responding during specified timeout <br />
!obj_not_found!         - a requested access point is not found <br />
!>_not_supported!       - read operation is not supported <br />
!<_not_supported!       - write operation is not supported <br />
!protocol_error!        - request message doesn't fit to a protocol format (missing access point name or access operator or a value) <br />


examples:

1. setting a DACA value to 5 Volts:

request message:                DACA<5.0\n
successive response message:     5.0\n

2. reading actual ADC2 raw value:

request message:                ADC1.raw>\n
successive response message:     3.7\n

3. setup a board via a JSON command:

request message:                js<{ {"gain", 3}, {"bridge", true}, {"offsets", {{"DACA", 0.1},{"DACB", -1.2},{"DACC", 0.5},{"DACD", 1.6}}} }\n
successive response message:     { {"gain", 3}, {"bridge", true}, {"offsets", {{"DACA", 0.1},{"DACB", -1.2},{"DACC", 0.5},{"DACD", 1.6}}} }\n

4. read back board settings via a JSON command:

request message:                js>{ {"gain", 0}, {"bridge", false}, {"DACA",0},{"DACB",0},{"DACC",0},{"DACD",0} }\n
successive response message:     { {"gain", 3}, {"bridge", true}, {"DACA", 0.1},{"DACB", -1.2},{"DACC", 0.5},{"DACD", 1.6} }\n



The protocol is implemented by the board's driver over a specific SPI protocol.







