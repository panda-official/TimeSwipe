# Data Model

The board's internal data is represented as access points or variables.
Each variable holds an only single value that can be written (set) to the variable or read out (get) if appropriate access rights are set (r/w)
The value is either primitive type (bool, int, float, etc) or an object.

Each variable(access point) has its own unique name that is written in the domain name form for convenience. Where the elder domain name is on the left.
For example: "ADC1.raw" means raw value of ADC channel 1.
This way access points form a hierarchical data model.

## Possible access point types:

### Type AOUT: 

This access point type is used to control the board's analog outputs and consists of two domain names.

Root domain:          Can be used only with sub-domain <br />
<br />
Sub domain (.raw):    Holds an analog output setpoint in a raw binary format (integer value 0:4095 discrets, r/w)

Here is a list of all possible analog output access points:

AOUT1.raw <br />
AOUT2.raw <br />
AOUT3.raw <br />
AOUT4.raw <br />

By the board design analog outputs #3 and #4 are shared by two DAC(digital to analog converter) chips. So it is possible to use either first chip or second to set a voltage on the analog output. The corresponding chip is selected by a DACsw variable (explained later in this document): 0 default chip(4-channel MAX5715), 1 - alternative chip(2-channel SAME54 internal DAC).
Note that analog output channels #1 and #2 are always connected to MAX 5715 #1 and #2 channels.
And analog output channels  #3 and #4 can be connected either to MAX 5715 #3 and #4 channels (default) or to #1 and #2 SAME54 DAC #1 and #2 channels.
While the chip is disconnected, its in the "cold" state. To preset the output values of currently disconnected ("cold") chip
additional access points are presented:

AOUT3cold.raw <br />
AOUT4cold.raw <br />


### Type ADC: 

This access point type is used to control board's ADCs and consists of two domain names.

Root domain:          Can be used only with sub-domain <br />
Sub domain (.raw):    Holds an ADC measured value in a raw binary format (integer value 0:4095 discrets, r)

Here is a list of all possible ADC's access points:

ADC1.raw <br />
ADC2.raw <br />
ADC3.raw <br />
ADC4.raw <br />


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
--------------  |    ------------------------------------------------------------------------------------------------------- 
Gain            |   Holds a gain value (integer value 1:4, r/w)
Bridge          |   Holds a bridge switch state (ON or OFF) (boolean, false:true, r/w)
Record          |   Writing "true" to this variable initiates/restarts a record process (boolean, false:true, r/w) 
Zero            |   Start/stop zero calibration process (boolean, false:true, w)
Zero.errtol     |   Holds a zero calibration process error tolerance value (integer r/w)
EnableADmes     |   Holds an ADC enabled state (ON or OFF) (boolean, false:true, r/w)
DACsw           |   Determines which of the DACs chip is connected to the analog outputs #3-4 (0 - default, 1 - alternative) (integer value, 0:1, r/w)

### JSON controlled access points:

 Access Point   |       Function
--------------  |    --------------------------------------------------------------------------------------------------------------------------------------- 
js              |    ("JSON setpoint") Writing to this variable a JSON object leads to write operation on multiple variables pointed in this object. The result of the operation will be returned as a JSON object (see the protocol description below). Reading from this variable a JSON object leads to readout values from all of the pointed variables as a JSON object (JSON object, r/w).      
je              |    ("JSON event") Holds latest events description in form of a JSON object (JSON object, r)

The structure of the JSON can be arbitrary but must follow a semantic rule: {"variable name" : value}. When reading information from a variable, the value is ignored and should be set to a question mark.

#### Examples: 

{
  "AOUT1.raw" : 2048,
  "AOUT2.raw" : 3000,
  "AOUT3.raw" : 1700,
  "AOUT4.raw" : 2200
}
                - a JSON object for setting a group of the 4 DACs
                

{
  "AOUT1.raw" : "?",
  "AOUT2.raw" : "?",
  "AOUT3.raw" : "?",
  "AOUT4.raw" : "?"
}
                - a JSON object for read back values of a group of the 4 DACs


# Communication protocols

There are a number of protocols that can be used for managing data of access points.
The simple ANSI text protocol is implemented by default.
The protocol works according to a master-slave communication model.

A request from a master consists of: <br />
__access point name (variable) + access operator ('>' means read variable, '<' means write value) + value (object) + new line character (/n)__  <br />
<br />
The response from a slave is a read back value in the case of success or an error message started with a '!' character.

### Common communication errors for all access points:

Error message       |    Meaning
------------------  |   -----------------------------------------------------------------------------------------------------------------
!Line_err!          |   communication bus error
!Timeout_err!       |   the board is not responding during specified timeout
!obj_not_found!     |   a requested access point is not found
!>_not_supported!   |   read operation is not supported
!<_not_supported!   |   write operation is not supported
!protocol_error!    |   request message does not fit to protocol format (e.g. missing access point name, access operator or value)


#### Examples:


##### 1. Setting an analog output #1 value to 2048 Discrets:

Request/Response             |  Command
---------------------------- | --------------------
request message:             |   AOUT1.raw<2048\n
successive response message: |   2048\n

<br />

##### 2. Preset a currently disconnected DAC's chip ("cold" state) output to 3000 Discrets to be ready connected to analog output #3 (DACsw=0, MAX5715 DAC channels #3,#4 connected to analog outputs #3,#4, SAME54 DAC disconnected):

Request/Response             |  Command
---------------------------- | --------------------
request message:             |   AOUT3cold.raw<3000\n
successive response message: |   3000\n

<br />

##### 3. Swap DAC chips connection to analog outputs #3 and #4 (will be: DACsw=1, MAX5715 DAC channels #3 and #4 disconnected, preset SAME54 DAC channels #1 & #2 connected to analog outputs #3 and #4):

Request/Response             |  Command
---------------------------- | --------------------
request message:             |   DACsw<1\n
successive response message: |   1\n

<br />

##### 4. Control analog output #3 directly with currently connected DAC chip (DACsw=1, means MAX5715 DAC channels #3 and #4 disconnected, SAME54 DAC channels #1 & #2 connected to analog outputs #3 and #4):

Request/Response             |  Command
---------------------------- | --------------------
request message:             |   AOUT3.raw<3000\n
successive response message: |   3000\n

<br />



##### 5. Reading actual ADC2 raw value:

Request/Response             |  Command
---------------------------- | --------------------
request message:             |   ADC1.raw>\n
successive response message: |    3.7\n

<br />

##### 6. Setup a board via a JSON command:

Request/Response              |  Command
----------------------------- | -------------------------------------------------------------------------------------------------------------------------
request message:              |   js<{ "Gain" : 3, "Bridge" : true,   "AOUT1.raw" : 500, "AOUT2.raw" : 700, "AOUT3.raw" : 900, "AOUT4.raw" : 1100 }\n
successive response message:  |       {"Gain" : 3, "Bridge" : true,   "AOUT1.raw" : 500, "AOUT2.raw" : 700, "AOUT3.raw" : 900, "AOUT4.raw" : 1100 }\n

<br />

##### 7. Read back board settings via a JSON command:

Request/Response               |  Command
------------------------------ | -------------------------------------------------------------------------------------------------------------
request message:               |  js>{ "Gain" : "?", "Bridge" : "?", "AOUT1.raw" : "?", "AOUT2.raw" : "?", "AOUT3.raw" : "?", "AOUT4.raw" : "?" }\n
successive response message:   |     {"Gain" : 3, "Bridge" : true,   "AOUT1.raw" : 500, "AOUT2.raw" : 700, "AOUT3.raw" : 900, "AOUT4.raw" : 1100 }\n

Note: When reading information from a variable via "js>" command, values in the pair "key:value" are ignored and should be set to a question mark. <br />

<br />

##### 8. Polling latest board events via a JSON command:

Request/Response               |  Command
------------------------------ | -------------------------------------------------------------------------------------------------------------------------
request message:               |  je>
successive response message:   |  { "Button" : true, "ButtonStateCnt" : 3 } - indicates the board's button was pressed and shows its state counter: odd value means the button is pressed, even means it is released.


The protocol is implemented by the board's driver over a specific SPI protocol.







