# Data Model

The board's internal data is represented as access points or variables.
Each variable holds an only single value that can be written (set) to the variable or read out (get) if appropriate access rights are set (r/w)
The value is either primitive type (bool, int, float, etc) or an object.

Each variable(access point) has its own unique name that is written in the domain name form for convenience. Where the elder domain name is on the left.
For example: "ADC1.raw" means raw value of ADC channel 1.
This way access points form a hierarchical data model.

## Possible access point types:

### Type DAC: 

This access point type is used to control the board's DACs and consists of two domain names.

Root domain:        Holds a DAC setpoint in floating-point format (real value, -10:+10 Volts, r/w)
<br />
Sub domain (.raw):    Holds a DAC setpoint in a raw binary format (integer value 0:4095 discrets, r/w)

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

DACA-D are external DACs on the board, while DAC1 and DAC2 are internal DACs of the ATSAME54P20A chip. Normally only the setting of DACA-D is required. <br />
Setting DAC1 and DAC2 is a special feature for controlling two of the analog outputs via rPi. For using this feature DACsw has to be set to 1 (explained later in this document).

#### Examples: 
"DACA"        an access point for board's DACA setpoint in floating-point format <br />
"DACA.raw"    an access point for board's DACA setpoint in raw binary format


### Type ADC: 

This access point type is used to control board's ADCs and consists of two domain names.

Root domain:        Can be used only with sub-domain <br />
Sub domain (.raw):    Holds an ADC measured value in a raw binary format (integer value 0:4095 discrets, r)

Here is a list of all possible ADC's access points:

ADC1.raw <br />
ADC2.raw <br />
ADC3.raw <br />
ADC4.raw <br />

#### Examples:
"ADC1.raw"   an access point for an actual measured value of board's ADC1 in raw binary format (0-4095, discrets)


### Type LED: 

This access point type is used to control board's LEDs.

Root domain:        Holds a LED ON/OFF state (boolen, false:true, w) <br />
Sub domain (.blink):   Holds a LED blinking mode (ON or OFF) (boolen, false:true, w) <br />
Sub domain (.col):     Holds a LED color value (integer RGB value 0:0xffffff, w)

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


### Access points with only one root domain name:

 Access Point   |       Function
--------------      --------------------------------------------------------------------------------------------------------------------------------------- 
Gain            |   Holds a gain value (integer value 1:4, r/w)
Bridge          |   Holds a bridge switch state (ON or OFF) (boolean, false:true, r/w)
Record          |   Writing "true" to this variable initiates/restarts a record process (boolean, false:true, r/w) 
Zero            |   Start/stop zero calibration process (boolean, false:true, w)
Zero.errtol     |   Holds a zero calibration process error tolerance value (integer r/w)
EnableADmes     |   Holds an ADC enabled state (ON or OFF) (boolean, false:true, r/w)
DACsw           |   Holds a DACs mode switch state (0 - using external DACs(A-C) only or 1 -using DACA, DACC, DAC0, DAC1) (integer value, 0:1, r/w)


### JSON controlled access points:

 Access Point   |       Function
--------------      --------------------------------------------------------------------------------------------------------------------------------------- 
js              |    Writing to this variable a JSON object leads to write operation on multiple variables pointed in this object. The result of the operation will be returned as a JSON object (see the protocol description below). Reading from this variable a JSON object leads to readout values from all of the pointed variables as a JSON object (JSON object, r/w).      
je              |    Holds latest events description in a form of a JSON object (JSON object, r)

The structure of the JSON can be arbitrary but must follow a semantic rule: {"variable name" : value}. When reading information from a variable, the value is ignored and should be set to a question mark.

#### Examples: 

{
  {"DACA" : 0.1},
  {"DACB" : -1.2},
  {"DACC" : 0.5},
  {"DACD" : 1.6}
}
                - a JSON object for setting a group of the 4 DACS
                

{
  {"DACA" : "?"},
  {"DACB" : "?"},
  {"DACC" : "?"},
  {"DACD" : "?"}
}
                - a JSON object for read back values of a group of the 4 DACS


# Communication protocols

There are a number of protocols that can be used for managing data of access points.
The simple ANSI text protocol is implemented by default.
The protocol works according to a master-slave communication model.

A request from a master consists of: access point name (variable) + access operator ('>' means read variable, '<' means write value) + value (object) + new line character (/n). 
The response from a slave is a read back value in the case of success or an error message started with a '!' character.

### Common communication errors for all access points:

Error message       |    Meaning
------------------     -----------------------------------------------------------------------------------------------------------------
!Line_err!          |   communication bus error
!Timeout_err!       |   the board is not responding during specified timeout
!obj_not_found!     |   a requested access point is not found
!>_not_supported!   |   read operation is not supported
!<_not_supported!   |   write operation is not supported
!protocol_error!    |   request message doesn't fit to a protocol format (missing access point name or access operator or a value)


#### Examples:

##### 1. setting a DACA value to 5 Volts:

Request/Response             |  Command
----------------------------   --------------------
request message:             |   DACA<5.0\n
successive response message: |    5.0\n

##### 2. reading actual ADC2 raw value:

Request/Response             |  Command
----------------------------   --------------------
request message:             |   ADC1.raw>\n
successive response message: |    3.7\n

##### 3. setup a board via a JSON command:

Request/Response              |  Command
-----------------------------   -------------------------------------------------------------------------------------------------------------------------
request message:              |   js<{ {"Gain" : 3}, {"Bridge" : true}, {"Offsets" : {{"DACA" : 0.1},{"DACB" : -1.2},{"DACC" : 0.5},{"DACD" : 1.6}}} }\n
successive response message:  |    { {"Gain" : 3}, {"Bridge" : true}, {"Offsets" : {{"DACA" : 0.1},{"DACB" : -1.2},{"DACC" : 0.5},{"DACD" : 1.6}}} }\n

##### 4. read back board settings via a JSON command:

Request/Response               |  Command
------------------------------   -------------------------------------------------------------------------------------------------------------
request message:               |  js>{ {"Gain" : "?"}, {"Bridge" : "?"}, {"DACA" : "?"}, {"DACB" : "?"}, {"DACC" : "?"}, {"DACD" : "?"} }\n
successive response message:   |   { {"Gain" : 3}, {"Bridge" : true}, {"DACA" : 0.1}, {"DACB" : -1.2}, {"DACC" : 0.5}, {"DACD" : 1.6} }\n

Note: When reading information from a variable via "js>" command values in the pair "key:value" are ignored and should be set to a question mark. <br />

##### 5. polling latest board events via a JSON command:

Request/Response               |  Command
------------------------------   -------------------------------------------------------------------------------------------------------------------------
request message:               |  je>
successive response message:   |  { {"Button" : true}, {"ButtonStateCnt" : 3} } - indicates the board's button was pressed and shows its state counter: odd value means the button is pressed, even means it is released.


The protocol is implemented by the board's driver over a specific SPI protocol.







