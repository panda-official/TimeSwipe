# Data Model

The board's internal data is represented as *access points* or *variables*. Each
variable holds a value of some primitive type (`bool`, `int`, `float`, etc) or a
JSON object. Some variables are both read/write and the others are read-only.

Each access point is uniquely named in the domain name form for convenience, where
the elder domain name is on the left. For example, `ADC1.raw` means raw value of
ADC channel 1. This way access points forms a hierarchical *data model*.

## Access points

### AOUT access point

By default, the input signal amplifier outputs connected to board's analog outputs.
But it's possible to control analog outputs `#3` and `#4` manually by selecting
the special control mode via the AOUT root domain.
The mode is activated by setting `AOUT` to `1`. Amplifier outputs `#3` and `#4` will
be disconnected and replaced by internal 2-channel DAC output. To control analog output
values `#3` and `#4` in this mode additional access points are presented.

|Domain         |Description                                            |Valid values                           |Access|
|:--------------|:------------------------------------------------------|:--------------------------------------|:-----|
|`AOUT3`/`AOUT4`|Globally enables/disables the analog output mode.      |bool                                   |r/w   |
|.raw           |Holds an analog output setpoint in a raw binary format.|int `[0,4095]`                         |r/w   |
|.pwm.repeats   |Holds a number of periods to generate.                 |unsigned `[0,0xffffffff]`, `0`=Infinite|r/w   |
|.pwm.duty      |Holds the PWM duty cycle (pulse width).                |float `[0.001,0.999]`                  |r/w   |
|.pwm.freq      |Holds a frequency to generate.                         |unsigned `[1,1000]`                    |r/w   |
|.pwm.high      |Holds the high PWM output level.                       |unsigned `[0,4095]`                    |r/w   |
|.pwm.low       |Holds the low PWM output level.                        |unsigned `[0,4095]`                    |r/w   |

### CH access point

This access point is used to control board's channel.

|Domain     |Description                                   |Valid values                                              |Access|
|:----------|:---------------------------------------------|:---------------------------------------------------------|:-----|
|`CH1`-`CH4`|Can be used only with a sub-domain.           |                                                          |      |
|.mode      |Measurement mode of the channel.              |`0` = Voltage mode, `1` = Current mode                    |r/w   |
|.gain      |Gain setting of the channel.                  |float `[1, 1408]` See [Gain Table](TechnicalSpecs.md#gain-settings)|r/w   |
|.iepe      |State of IEPE switch of the channel.          |bool                                                      |r/w   |
|.offset    |Offset Setting of the channel                 |unsigned `[0,4095]`                                       |r/w   |
|.armadc    |Raw value of the 12bit ARM ADC                |unsigned `[0,4095]`                                       |r     |

The `.mode`, `.gain`, `.iepe` and `.offset` access points are only available in
TimeSwipe 1.0+ boards.

### Fan access point

This access point is used to control board's Fan output.

|Domain|Description                                |Valid values           |Access|
|:-----|:------------------------------------------|:----------------------|:-----|
|`Fan` |Enables or disables Fan globally.          |bool                   |r/w   |
|.duty |Fan PWM duty cycle (pulse width).          |float `[.001,.999]`    |r     |
|.freq |Fan PWM frequency.                         |unsigned `[1,20000]`   |r/w   |

### Supply access point

This access point is used to control board's Supply Voltage Output.

|Domain   |Description                                |Valid values      |Access|
|:--------|:------------------------------------------|:-----------------|:-----|
|`Supply` |Enables or disables Supply globally.       |bool              |r/w   |
|.voltage |Output Voltage setting                     |float `[2.5,24]`  |r/w   |

The `.voltage` access point is only available in TimeSwipe 1.0+ boards. On older
boards the voltage is fixed to `24VDC`.

### Calibration access point
This access point is used to get and set board's calibration values.

|Domain        |Description                                |Valid values      |Access|
|:-------------|:------------------------------------------|:-----------------|:-----|
|`Calibration` |Returns if board is calibrated or not      |bool              |r|
|`.version`    |Returns the version of the calibration     |string            |r|
|`.date`       |Returns the date of the calibration        |string            |r|
|`.cAtom`      |Access to the cAtoms                       |cAtom object      |r|

The `.voltage` access point is only available in TimeSwipe 1.0+ boards. On older
boards the voltage is fixed to `24VDC`.
The Calibration cAtom structure is described in the Calibration Atom Specification
This data is only r/w in calibration firmware.

### Access points with only root domain

|Access point |Description |Valid values|Access|
|:------------|:-----------|:-----------|:-----|
|Record       |Writing "1" to this variable initiates/restarts a record process.|bool                                         |r/w|
|Offset       |Starts/stops offset searching process.|int, `0`=Stop, `1`=Negative offset, `2`=Zero offset, `3`=Positive offset|r/w|
|Offset.errtol|Holds a calibration process error tolerance value.|int                                                         |r/w|
|EnableADmes  |Holds an ADC enabled state (ON or OFF).|bool                                                                   |r/w|
|Temp         |Holds the current core temperature of ARM Chip in degrees Celsius.|float                                       |r|
|ARMID		  |Holds Hardware Chip ID.|string                                                                                 |r|
|fwVersion	  |Holds firmware Version in the SemVer format.|string                                                            |r|

The following access points are only available on older Boards prior to Timeswipe 1.0:

|Access point |Description |Valid values|Access|
|:------------|:-----------|:-----------|:-----|
|Mode         |Sets working mode of the board.|`0` = IEPE, `1` = Normal Signal, `2` = Digital|r/w|
|IEPE         |Enables IEPE current output globally|bool                                     |r/w|

### JSON controlled access points

|Access point|Description|Valid values|Access|
|:-----------|:----------|:-----------|:-----|
|js          |"JSON setpoint". Writing to this variable a JSON string leads to write operation on multiple variables given in the string. The result of the operation will be returned as a JSON object (see the [protocol description](#communication-protocol)). Reading from this variable a JSON object leads to readout values from all of the given variables as a JSON object.|JSON object|r/w|
|je          |"JSON event". Holds the latest events description as JSON.|JSON object|r|

The structure of the JSON object can be arbitrary but must follow the following semantic rules:
  - when writing to a variable the format of JSON object entry should be: `{"variable" : value}`;
  - when reading from a variable it's name should be inserted to a "JSON array" type: ["variable", ...] (preferable);
  - when reading from a variable the format of JSON object entry should be: `{"variable" : "?"}` (alternative way of the above).

**Note, for all JSON controlled access points: writing a request with a call of
a JSON command inside ("js" or "je") leads to "disabled" error!**

## Communication protocol

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
CH1.offset<2048\n
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

if `AOUT` is set to `1`, amplifier's outputs `#3` and `#4` are disconnected,
preset channels `AOUT3` and `AOUT4` are connected to analog outputs `#3` and `#4`.

#### Request

```
AOUT<1\n
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
CH2.armadc>\n
```

#### Response

```
2048\n
```

### 6. Setup a board via a JSON command

#### Request

```
js<{ "CH1.Gain" : 3, "Supply" : true, "CH1.offset" : 500, "CH2.offset" : 700, "CH3.offset" : 900, "CH4.offset" : 1100 }\n
```

#### Response

```
{"CH1.Gain" : 3, "Supply" : true, "CH1.offset" : 500, "CH2.offset" : 700, "CH3.offset" : 900, "CH4.offset" : 1100 }\n
```

### 7. Read back board settings via a JSON command (preferable way)

#### Request

```
js>[ "CH1.Gain", "Supply", "CH1.offset", "CH2.offset", "CH3.offset", "CH4.offset" ]\n
```

#### Response

```
{"CH1.Gain" : 3, "Supply" : true, "CH1.offset" : 500, "CH2.offset" : 700, "CH3.offset" : 900, "CH4.offset" : 1100 }\n
```

### 8. Read back board settings via a JSON command (alternative way)

#### Request

```
js>{ "CH1.Gain" : "?", "Supply" : "?", "CH1.offset" : "?", "CH2.offset" : "?", "CH3.offset" : "?", "CH4.offset" : "?" }\n
```

#### Response

```
{"CH1.Gain" : 3, "Supply" : true, "CH1.offset" : 500, "CH2.offset" : 700, "CH3.offset" : 900, "CH4.offset" : 1100 }\n
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
{"CH1.armadc" : 2047, ...<all previous listed variables with a read access>... }\n
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
js>[ "CH1.armadc", "CH2.armadc", "js" ]

{"CH1.armadc":2107,"CH2.armadc":2041,"js":{"error":{"edescr":"disabled!","val":""}}}
```
