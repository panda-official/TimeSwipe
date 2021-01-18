# Data Model

The board's internal data is represented as *access points* or *variables*. Each
variable holds a value of some primitive type (`bool`, `int`, `float`, etc) or a
JSON object. Some variables are both read/write and the others are read-only.

Each access point is uniquely named in the domain name form for convenience, where
the elder domain name is on the left. For example, `ADC1.raw` means raw value of
ADC channel 1. This way access points forms a hierarchical *data model*.

## Access points

### DAC access point

This access point is used for setting offset on the input signal for each of `4`
channels by controlling the input signal amplifier. The access point consists of
two domain names.

|Domain|Description                                            |Valid values  |Access|
|:-----|:------------------------------------------------------|:-------------|:-----|
|root  |Can be used only with `.raw`.                          |              |      |
|.raw  |Holds an analog output setpoint in a raw binary format.|int `[0,4095]`|r/w   |

The possible DAC access points are:

  - `DAC1.raw`
  - `DAC2.raw`
  - `DAC3.raw`
  - `DAC4.raw`

### AOUT access point

By default, the input signal amplifier outputs connected to board's analog outputs.
But it's possible to control analog outputs `#3` and `#4` manually by selecting
the special control mode via the `DACsw` variable (explained later in this document).
The mode is activated by setting `DACsw` to `1`. Amplifier outputs `#3` and `#4` will
be disconnected and replaced by internal 2-channel DAC output. To control analog output
values `#3` and `#4` in this mode additional access points are presented.

|Domain|Description                                            |Valid values  |Access|
|:-----|:------------------------------------------------------|:-------------|:-----|
|root  |Can be used only with `.raw`                           |              |      |
|.raw  |Holds an analog output setpoint in a raw binary format.|int `[0,4095]`|r/w   |

The possible AOUT access points are:

  - `AOUT3.raw`
  - `AOUT4.raw`

### ADC access point

This access point is used to control board's ADCs and consists of two domain names.

|Domain|Description                                        |Valid values  |Access|
|:-----|:--------------------------------------------------|:-------------|:-----|
|root  |Can be used only with `.raw`.                      |              |      |
|.raw  |Holds an ADC measured value in a raw binary format.|int `[0,4095]`|r     |

The possible ADC access points are:

  - `ADC1.raw`
  - `ADC2.raw`
  - `ADC3.raw`
  - `ADC4.raw`

### PWM access point

This access point is used to control board's `AOUT3` and `AOUT4` PWMs.

|Domain  |Description                            |Valid values                           |Access|
|:-------|:--------------------------------------|:--------------------------------------|:-----|
|root    |Holds the PWMx ON/OFF state.           |bool                                   |r/w   |
|.repeats|Holds a number of periods to generate. |unsigned `[0,0xffffffff]`, `0`=Infinite|r/w   |
|.duty   |Holds the PWM duty cycle (pulse width).|float `[0.001,0.999]`                  |r/w   |
|.freq   |Holds a frequency to generate.         |unsigned `[1,1000]`                    |r/w   |
|.high   |Holds the high PWM output level.       |unsigned `[0,4095]`                    |r/w   |
|.low    |Holds the low PWM output level.        |unsigned `[0,4095]`                    |r/w   |

The possible PWM access points are:

  - `PWM1`
  - `PWM1.repeats`
  - `PWM1.duty`
  - `PWM1.freq`
  - `PWM1.high`
  - `PWM1.low`
  - `PWM2`
  - `PWM2.repeats`
  - `PWM2.duty`
  - `PWM2.freq`
  - `PWM2.high`
  - `PWM2.low`

### CH access point

This access point is used to control board's channel.

|Domain|Description                                   |Valid values                               |Access|
|:-----|:---------------------------------------------|:------------------------------------------|:-----|
|root  |Can be used only with a sub-domain.           |bool                                       |r/w   |
|.mode |Holds current measurement mode of the channel.|unsigned `0`=Voltage mode, `1`=Current mode|r/w   |
|.gain |Holds current Gain setting of the channel.    |float `[1/8, 128*1.375]`                   |r/w   |
|.iepe |Holds the state of IEPE switch of the channel.|bool                                       |r/w   |

The possible CH access points are:

  - `CH1.mode`
  - `CH1.gain`
  - `CH1.iepe`
  - `CH2.mode`
  - `CH2.gain`
  - `CH2.iepe`
  - `CH3.mode`
  - `CH3.gain`
  - `CH3.iepe`
  - `CH4.mode`
  - `CH4.gain`
  - `CH4.iepe`

### Fan access point

This access point is used to control board's Fan output.

|Domain|Description                                |Valid values                               |Access|
|:-----|:------------------------------------------|:------------------------------------------|:-----|
|root  |Enables or disables Fan globally.          |bool                                       |r/w   |
|.duty |Holds the Fan PWM duty cycle (pulse width).|float `[.001,.999]`                        |r     |
|.freq |Holds the Fan PWM frequency.               |unsigned `[1,20000]`                       |r/w   |

### Access points with only root domain

|Access point |Description |Valid values|Access|
|:------------|:-----------|:-----------|:-----|
|Gain         |Holds Gain value.|int `[1,4]`|r/w|
|Bridge       |Holds Bridge Switch state (ON or OFF).|bool|r/w|
|Record       |Writing "1" to this variable initiates/restarts a record process.|bool|r/w|
|Mode         |Sets working mode of the board.|int, `0`=IEPE, `1`=Normal Signal, `2`=Digital|r/w|
|Offset       |Starts/stops offset searching process.|int, `0`=Stop, `1`=Negative offset, `2`=Zero offset, `3`=Positive offset|r/w|
|Offset.errtol|Holds a calibration process error tolerance value.|int|r/w|
|EnableADmes  |Holds an ADC enabled state (ON or OFF).|bool|r/w|
|DACsw        |Determines the mode of controlling analog outputs `#3`, `#4`.|int, `0`=default (amplified input signal), `1`=manual via `AOUT3`, `AOUT4`|r/w|
|Temp         |Holds the current core temperature of `SAME54` in degrees Celsius.|float|r|
|ARMID		  |Holds Hardware Chip ID.|string|r|
|fwVersion	  |Holds firmware Version in the SemVer format.|string|r|
|CalStatus    |Holds board calibration status.|bool|r|
|Voltage      |Holds Output Voltage value (mockup).|float|r/w|
|Current      |Holds Current Setting (mockup).|float, `0`=MaxCurrent|r/w|
|MaxCurrent   |Holds MaxCurrent Setting (Current max range, mockup).|float|r/w|

### JSON controlled access points

|Access point|Description|Valid values|Access|
|:-----------|:----------|:-----------|:-----|
|js          |"JSON setpoint". Writing to this variable a JSON object leads to write operation on multiple variables given in this object. The result of the operation will be returned as a JSON object (see the protocol description below). Reading from this variable a JSON object leads to readout values from all of the given variables as a JSON object.|JSON object|r/w|
|je          |"JSON event". Holds the latest events description in form of the JSON object.|JSON object|r|

The structure of the JSON object can be arbitrary but must follow the following semantic rules:
  - when writing to a variable the format of JSON object entry should be: `{"variable" : value}`;
  - when reading from a variable it's name should be inserted to a "JSON array" type: ["variable", ...] (preferable);
  - when reading from a variable the format of JSON object entry should be: `{"variable" : "?"}` (alternative way of the above).

**Note, for all JSON controlled access points: writing a request with a call of a
JSON command inside ("js" or "je") leads to "disabled" error!**

## Communication protocols

The protocol is implemented by the board's driver over a specific SPI protocol.
There are a number of protocols that can be used for managing data of access points.
The simple ANSI text protocol is implemented by default. The protocol works according
to a *master/slave* communication model. The syntax of the request from a *master* is
as follows:

```
access-point access-operator value\n
```

where

- `access-point` - is a name of an access point (variable);
- `access-operator` - `>` (read value) or `<` (write value);
- `value` - a value of the primitive type or a JSON object.

The response from a *slave* is a readback value in case of success, or an error message
started with the `!` character.

## Examples

### 1. Setting an offset value for channel #1 to 2048 Discrets

#### Request

```
DAC1.raw<2048\n
```

#### Response

```
2048\n
```

### 2. Preset a value for analog output #3 to 2048 Discrets to be controlled in the manual mode

#### Request

```
AOUT3.raw<2048\n
```

#### Response

```
2048\n
```

### 3. Set the manual control mode for channels #3 and #4

#### Effects

`DACsw` is set to `1`, amplifier's outputs `#3` and `#4` are disconnected,
preset channels `AOUT3` and `AOUT4` are connected to analog outputs `#3` and `#4`.

#### Request

```
DACsw<1\n
```

#### Response

```
1\n
```

### 4. Control the analog output #4 directly in the manual mode (DACsw=1)

#### Request

```
AOUT4.raw<3000\n
```

#### Response

```
3000\n
```

### 5. Reading actual ADC2 raw value

#### Request

```
ADC2.raw>\n
```

#### Response

```
2048\n
```

### 6. Setup a board via a JSON command

#### Request

```
js<{ "Gain" : 3, "Bridge" : true,   "DAC1.raw" : 500, "DAC2.raw" : 700, "DAC3.raw" : 900, "DAC4.raw" : 1100 }\n
```

#### Response

```
{"Gain" : 3, "Bridge" : true,   "DAC1.raw" : 500, "DAC2.raw" : 700, "DAC3.raw" : 900, "DAC4.raw" : 1100 }\n
```

### 7. Read back board settings via a JSON command (preferable way)

#### Request

```
js>[ "Gain", "Bridge", "DAC1.raw", "DAC2.raw", "DAC3.raw", "DAC4.raw" ]\n
```

#### Response

```
{"Gain" : 3, "Bridge" : true,   "DAC1.raw" : 500, "DAC2.raw" : 700, "DAC3.raw" : 900, "DAC4.raw" : 1100 }\n
```

### 8. Read back board settings via a JSON command (alternative way)

#### Request

```
js>{ "Gain" : "?", "Bridge" : "?", "DAC1.raw" : "?", "DAC2.raw" : "?", "DAC3.raw" : "?", "DAC4.raw" : "?" }\n
```

#### Response

```
{"Gain" : 3, "Bridge" : true,   "DAC1.raw" : 500, "DAC2.raw" : 700, "DAC3.raw" : 900, "DAC4.raw" : 1100 }\n
```

Note: when reading from variables with `js>` command the question marks ("?")
must be used instead of values of "key:value" pairs.

### 9. Dump all available variables with a read access via a single JSON command

#### Request

```
js>\n
```

#### Response

```
{"ADC1.raw" : 2047, ...<all previous listed variables with a read access>... }\n
```

### 10. Polling the latest board events via a JSON command

#### Request

```
je>\n
```

#### Response

```
{"Button" : true, "ButtonStateCnt" : 3 }
```

Indicates the board's button was pressed and shows its state counter:
  - odd value means the button is pressed;
  - even value means it is released.

Note: the event response message can vary depending on the current events active
(please, see [the Event System documentation](EventSystem.md)).

### Common communication errors for all access points

|Error message    |Description|
|:----------------|:----------|
|!Line_err!       |Communication bus error.|
|!Timeout_err!    |The board is not responding during specified timeout.|
|!obj_not_found!  |A requested access point is not found.|
|!>_not_supported!|Read operation is not supported.|
|!<_not_supported!|Write operation is not supported.|
|!disabled!       |The access point is disabled.|
|!protocol_error! |Request message does not fit to protocol format. (E.g. missing access point name, access operator or value.)|
|!stoi            |String to integer value conversion error.|
|!stof            |String to floating point conversion error.|

### JSON specific communication errors

Upon processing a JSON request an error can be generated while processing each
entry. The error information is placed to the output JSON object in the following form:

```
{"variable" : {"edescr":"error message","val":"value"}}
```

where:
  - `variable` - a name of a variable;
  - `error message` - text of the *Error message* column of the table above
  without preceding "!", or specific syntax error generated by a JSON parser;
  - `value` - the value set in the incoming request.

Example:

```
js>[ "ADC1.raw", "ADC2.raw", "js" ]

{"ADC1.raw":2107,"ADC2.raw":2041,"js":{"error":{"edescr":"disabled!","val":""}}}
```
