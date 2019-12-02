# Data Model

The board's internal data is represented as access points or variables.
Each variable holds an only single value that can be written (set) to the variable or read out (get) if appropriate access rights are set (r/w)
The value is either primitive type (bool, int, float, etc) or an object.

Each variable(access point) has its own unique name that is written in the domain name form for convenience. Where the elder domain name is on the left.
For example: "ADC1.raw" means raw value of ADC channel 1.
This way access points form a hierarchical data model.

## Possible access point types:

### Type DAC: 

This access point type is used for setting offset on the input signal for each of 4 channels by controlling the input signal amplifier.
The access point consists of two domain names.

Root domain:          Can be used only with sub-domain <br />
<br />
Sub domain (.raw):    Holds an analog output setpoint in a raw binary format (integer value 0:4095 discrets, r/w)

Here is a list of all possible DAC access points:

DAC1.raw <br />
DAC2.raw <br />
DAC3.raw <br />
DAC4.raw <br />

### Type AOUT

By default the input signal amplifier outputs connected to board's analog outputs.
But there is an ability to control analog outputs #3 and #4 manually by selecting the special control mode via a DACsw variable (explained later in this document).
The mode is activated by setting DACsw=1. Amplifier outputs #3 and #4 will be disconnected and replaced by internal 2-channel DAC output.
To control analog output values #3 and #4 in this mode additional access points are presented:

Root domain:          Can be used only with sub-domain <br />
<br />
Sub domain (.raw):    Holds an analog output setpoint in a raw binary format (integer value 0:4095 discrets, r/w)


AOUT3.raw <br />
AOUT4.raw <br />


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
DACsw           |   Determines the mode of controlling analog outputs #3-4 (0 - default (amplified input signal), 1 - manual via AOUT3, AOUT4) (integer value, 0:1, r/w)

### JSON controlled access points:

 Access Point   |       Function
--------------  |    --------------------------------------------------------------------------------------------------------------------------------------- 
js              |    ("JSON setpoint") Writing to this variable a JSON object leads to write operation on multiple variables pointed in this object. The result of the operation will be returned as a JSON object (see the protocol description below). Reading from this variable a JSON object leads to readout values from all of the pointed variables as a JSON object (JSON object, r/w).      
je              |    ("JSON event") Holds latest events description in form of a JSON object (JSON object, r)

The structure of the JSON object can be arbitrary but must follow several semantic rules:
When setting a value in an entry of a JSON object the format should be {"variable name" : value}.
When reading information from a variable its name can be placed in a "JSON array" type: ["variable name", ..].
Its a preferable way. Alternativetly it can be requested with a "get" request in a following form {"variable name" : "?"}




# Communication protocols

There are a number of protocols that can be used for managing data of access points.
The simple ANSI text protocol is implemented by default.
The protocol works according to a master-slave communication model.

A request from a master consists of: <br />
__access point name (variable) + access operator ('>' means read variable, '<' means write value) + value (object) + new line character (/n)__  <br />
<br />
The response from a slave is a read back value in the case of success or an error message started with a '!' character.


#### Examples:


##### 1. Setting an offset value for channel #1 to 2048 Discrets:

Request/Response             |  Command
---------------------------- | --------------------
request message:             |   DAC1.raw<2048\n
successive response message: |   2048\n

<br />

##### 2. Preset a value for analog output #3 to 2048 Discrets to be controlled in a manual mode:

Request/Response             |  Command
---------------------------- | --------------------
request message:             |   AOUT3.raw<2048\n
successive response message: |   2048\n

<br />



##### 3. Set the manual control mode for channels #3 and #4 (will be: DACsw=1, amplifier's otputs #3 and #4 are disconnected, preset channels AOUT3 & AOUT4 connected to analog outputs #3 and #4):

Request/Response             |  Command
---------------------------- | --------------------
request message:             |   DACsw<1\n
successive response message: |   1\n

<br />

##### 4. Control analog output #4 directly in the manual mode (DACsw=1):

Request/Response             |  Command
---------------------------- | --------------------
request message:             |   AOUT4.raw<3000\n
successive response message: |   3000\n

<br />



##### 5. Reading actual ADC2 raw value:

Request/Response             |  Command
---------------------------- | --------------------
request message:             |   ADC1.raw>\n
successive response message: |    2048\n

<br />

##### 6. Setup a board via a JSON command:

Request/Response              |  Command
----------------------------- | -------------------------------------------------------------------------------------------------------------------------
request message:              |   js<{ "Gain" : 3, "Bridge" : true,   "DAC1.raw" : 500, "DAC2.raw" : 700, "DAC3.raw" : 900, "DAC4.raw" : 1100 }\n
successive response message:  |       {"Gain" : 3, "Bridge" : true,   "DAC1.raw" : 500, "DAC2.raw" : 700, "DAC3.raw" : 900, "DAC4.raw" : 1100 }\n

<br />

##### 7. Read back board settings via a JSON command(preferable way)

Request/Response               |  Command
------------------------------ | -------------------------------------------------------------------------------------------------------------
request message:               |  js>[ "Gain", "Bridge", "DAC1.raw", "DAC2.raw", "DAC3.raw", "DAC4.raw" ]\n
successive response message:   |     {"Gain" : 3, "Bridge" : true,   "DAC1.raw" : 500, "DAC2.raw" : 700, "DAC3.raw" : 900, "DAC4.raw" : 1100 }\n

<br />

##### 8. Read back board settings via a JSON command(alternative way)

Request/Response               |  Command
------------------------------ | -------------------------------------------------------------------------------------------------------------
request message:               |  js>{ "Gain" : "?", "Bridge" : "?", "DAC1.raw" : "?", "DAC2.raw" : "?", "DAC3.raw" : "?", "DAC4.raw" : "?" }\n
successive response message:   |     {"Gain" : 3, "Bridge" : true,   "DAC1.raw" : 500, "DAC2.raw" : 700, "DAC3.raw" : 900, "DAC4.raw" : 1100 }\n

Note: When reading information from a variable via "js>" command, values in the pair "key:value" are ignored and should be set to a question mark. <br />

<br />

##### 9. Dump all available variables with a read access via a single JSON command

Request/Response               |  Command
------------------------------ | -------------------------------------------------------------------------------------------------------------
request message:               |  js>\n
successive response message:   |  {"ADC1.raw" : 2047, ...<all previous listed variables with a read access>... }\n

<br />

##### 10. Polling latest board events via a JSON command:

Request/Response               |  Command
------------------------------ | -------------------------------------------------------------------------------------------------------------------------
request message:               |  je>\n
successive response message:   |  {"Button" : true, "ButtonStateCnt" : 3 } - indicates the board's button was pressed and shows its state counter: odd value means the button is pressed, even means it is released.

Note: The event response message can vary depending on the current events active (please, see "EventSystem" documentation)

__Note for all JSON commands: writing a request with a call of a JSON command inside("js" or "je") leads to "disabled" error__

<br />


### Common communication errors for all access points:

Error message       |    Meaning
------------------  |   -----------------------------------------------------------------------------------------------------------------
!Line_err!          |   communication bus error
!Timeout_err!       |   the board is not responding during specified timeout
!obj_not_found!     |   a requested access point is not found
!>_not_supported!   |   read operation is not supported
!<_not_supported!   |   write operation is not supported
!disabled!          |   the access point is disabled
!protocol_error!    |   request message does not fit to protocol format (e.g. missing access point name, access operator or value)

<br />


### JSON specific communication errors

While processing a JSON request an error can be generated while processing each entry. The error information will be placed in the output JSON object in the following form:

{"variable name" : {"edescr":"error description","val":"value"}}}.

where "error description" text error description from the table above without preceding "!" or specific syntax error generated by a JSON parser.
"value" the value set in the incoming request.

example: js>[ "ADC1.raw", "ADC2.raw", "js" ]
{"ADC1.raw":2107,"ADC2.raw":2041,"js":{"error":{"edescr":"disabled!","val":""}}}

<br />



The protocol is implemented by the board's driver over a specific SPI protocol.


