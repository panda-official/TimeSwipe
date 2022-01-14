# Introduction

The board settings can be modifiable or read-only. Each setting is uniquely
named in the following form:
```
<primary>[[<index>]<secondary>]
```
where `<primary>` - is a primary name (category), `<index>` - is an optional
integer of some range, `<secondary>` - is an optional secondary name.
For example, `channel1AdcRaw` means ADC raw value of channel 1.

## Settings

### Settings group `analogOut`

By default, the input signal amplifier controls connected to board's analog
outputs. However, it's possible to attach 2-channel DAC output to analog
outputs 3 and 4 by assigning `true` to the `analogOutsDacEnabled` setting, and
to control these analog outputs via `analogOut%DacRaw` settings.

|Name                |Description                  |Range   |Access|Default|
|:-------------------|:----------------------------|:-------|:-----|:------|
|analogOut%DacRaw    |Value in a raw binary format.|[0,4095]|rw    |2048   |
|analogOutsDacEnabled|Analog outputs DAC enabled?  |bool    |rw    |0      |

#### Details

- `%` - is an index in range `[3, 4]`;
- `analogOutsDacEnabled` range: `false` - disabled (amplified input signal),
`true` - enabled and controllable via `analogOut%DacRaw`.

### Settings group `channel`

These settings are used to control board channels.

|Name          |Description                |Range            |Access|Default|
|:-------------|:--------------------------|:----------------|:-----|:------|
|channel%AdcRaw|ADC measured value.        |[0,4095]         |r     |       |
|channel%DacRaw|Offset on the input signal.|[0,4095]         |rw    |2048   |
|channel%Mode  |Measurement mode.          |[0,1]            |rw    |0      |
|channel%Gain  |Gain value.                |[.125, 128*1.375]|rw    |1      |
|channel%Iepe  |IEPE?                      |bool             |rw    |false  |
|channelsAdcEnabled|ADC measurement enabled?|bool            |rw    |false  |
|channelsCalibrationValid|Calibration data valid?|bool       |r     |       |
|channelsCalibrationEnabled|Calibration data enabled?|bool   |rw    |false  |

#### Details

- `%` - is an index in range `[1, 4]`;
- `channel%Mode` range: `0` - voltage, `1` - current;
- `channel%AdcRaw`, `channel%DacRaw` - values are in a raw binary format.

### Settings group `fan`

These settings are used to control board Fan output.

|Name         |Description              |Range      |Access|Default|
|:------------|:------------------------|:----------|:-----|:------|
|fanEnabled   |Fan enabled?             |bool       |rw    |true   |
|fanDutyCycle |Duty cycle (pulse width).|[.001,.999]|r     |       |
|fanFrequency |Frequency.               |[1,20000]  |rw    |100    |

### Settings group `pwm`

These settings are used to control PWMs of analog outputs `3` and `4`.

|Name            |Description                   |Range         |Access|Default|
|:---------------|:-----------------------------|:-------------|:-----|:------|
|pwm%Enabled     |PWM enabled?                  |bool          |rw    |false  |
|pwm%RepeatCount |Number of periods to generate.|[0,4294967295]|rw    |0      |
|pwm%DutyCycle   |Duty cycle (pulse width).     |[0.001,0.999] |rw    |0.5    |
|pwm%Frequency   |Frequency to generate.        |[1,1000]      |rw    |50     |
|pwm%HighBoundary|High pulse boundary.          |[0,4095]      |rw    |3072   |
|pwm%LowBoundary |Low pulse boundary            |[0,4095]      |rw    |2048   |

#### Details

- `%` - is an index in range `[1, 2]`;
- `pwm%RepeatCount` range: `0` - infinite.

### Settings group `voltageOut`

These settings are used to control board voltage output.

|Name             |Description                   |Range     |Access|Default|
|:----------------|:-----------------------------|:---------|:-----|:------|
|voltageOutEnabled|Bridge mode enabled?          |bool      |rw    |false  |
|voltageOutValue  |Output value.                 |[2.5,24.0]|rw    |2.5    |

### Miscellaneous settings

|Name              |Description                              |Range |Access|Default|
|:-----------------|:----------------------------------------|:-----|:-----|:------|
|armId             |ARM chip UUID.                           |string|r     |       |
|firmwareVersion   |Firmware version.                        |string|r     |       |
|temperature       |Core temperature of ARM chip.            |float |r     |       |
|Gain              |Gain.                                    |[1,4] |rw    |1      |
|Record            |Starts/stops a record process.           |bool  |rw    |false  |
|Mode              |Working mode of the board.               |[0,2] |rw    |0      |
|Offset            |Starts/stops offset searching process.   |[0,3] |rw    |0      |
|Offset.errtol     |Offset searching process error tolerance.|int   |rw    |25     |
|Current           |Current.                                 |float |rw    |       |
|MaxCurrent        |Max Current.                             |float |rw    |1000   |

#### Details

- `temperature` - value is in Celsius;
- `Mode` range: `0` - IEPE, `1` - normal signal, `2`- digital;
- `Offset` range: `0` - stopped, `1` - negative, `2` - zero, `3` - positive;
- `Current`: `0` - is a back-reference to `MaxCurrent`.

### Special settings

#### Special setting `js`

Writing a JSON object to this setting affects multiple settings specified in
JSON object. The result of both writing or reading of this settings - is a JSON
object with actual settings.

The structure of the JSON object can be arbitrary but must follow the following
semantic rules:
  - when writing a setting the format of JSON object entry should be:
  `{"setting" : value}`;
  - when reading a setting it's name should be inserted to a JSON array:
  `["setting", ...]` (preferable);
  - when reading a setting the format of JSON object entry should be:
  `{"setting" : "?"}` (alternative).

**Note, that special settings are not allowed in the JSON object!**

#### Special setting `je`

The result of reading this setting - is a JSON object with latest events.

## Communication protocols

The syntax of the SPI request is the following:

```
setting operator value\n
```

where

- `setting` - is a setting name;
- `operator` - `>` (read) or `<` (write);
- `value` - a JSON value.

The SPI response is a readback value in case of success, or an error
message started with the `!` character.

### Examples

#### 1. Setting an offset value for channel 1 to 2048 discrets

##### Request

```
channel1DacRaw<2048\n
```

##### Response

```
2048\n
```

#### 2. Preset a value for analog output 3 to 2048 discrets to be controlled
in the manual mode

##### Request

```
analogOut3Raw<2048\n
```

##### Response

```
2048\n
```

#### 3. Activate the manual control over for analog outputs 3 and 4

##### Effects

Amplifier's outputs 3 and 4 are disconnected, preset channels `aout3` and
`aout4` are connected to analog outputs 3 and 4.

##### Request

```
analogOutsDacEnabled<true\n
```

##### Response

```
true\n
```

#### 4. Control the analog output 4 manually

##### Requires

`analogOutsDacEnabled` is `true`.

##### Request

```
analogOut4Raw<3000\n
```

##### Response

```
3000\n
```

#### 5. Reading actual adc2Raw value

##### Request

```
adc2Raw>\n
```

##### Response

```
2048\n
```

#### 6. Update settings via `js` special command

##### Request

```
js<{"Gain":3,"voltageOutEnabled":true,"channel1DacRaw":500,"channel2DacRaw":700,"channel3DacRaw":900,"channel4DacRaw":1100}\n
```

##### Response

```
{"Gain":3,"voltageOutEnabled":true,"channel1DacRaw":500,"channel2DacRaw":700,"channel3DacRaw":900,"channel4DacRaw":1100}\n
```

#### 7. Read back board settings via a JSON command (preferable way)

##### Request

```
js>["Gain","voltageOutEnabled","channel1DacRaw","channel2DacRaw","channel3DacRaw","channel4DacRaw"]\n
```

##### Response

```
{"Gain":3,"voltageOutEnabled":true,"channel1DacRaw":500,"channel2DacRaw":700,"channel3DacRaw":900,"channel4DacRaw":1100}\n
```

#### 8. Read back board settings via a JSON command (alternative way)

##### Request

```
js>{"Gain":"?","voltageOutEnabled":"?","channel1DacRaw":"?","channel2DacRaw":"?","channel3DacRaw":"?","channel4DacRaw":"?"}\n
```

##### Response

```
{"Gain":3,"voltageOutEnabled":true,"channel1DacRaw":500,"channel2DacRaw":700,"channel3DacRaw":900,"channel4DacRaw":1100}\n
```

Note: when reading settings with `js>` command the question marks ("?") must be
used instead of values of "key:value" pairs.

#### 9. Dump all available settings via a single JSON command

##### Request

```
js>\n
```

##### Response

```
{<all the settings>}\n
```

#### 10. Polling the latest board events via a JSON command

##### Request

```
je>\n
```

##### Response

```
{"Button":true, "ButtonStateCnt":3}
```

Indicates the board's button was pressed and shows its state counter:
  - odd value means the button is pressed;
  - even value means it is released.

Note: the event response message can vary depending on the current events active
(please, see [the Event System documentation](EventSystem.md)).

### SPI errors

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

#### `js` setting errors

Upon processing a JSON request an error can be generated while processing each
entry. The error information is placed to the output JSON object in the following
form:

```
{"setting" : {"edescr":"message","val":"value"}}
```

where:
  - `setting` - a name of a setting;
  - `edescr` - "edescr" literaly,
  - `message` - text of the *Error message* column of the table above,
  without preceding "!", or specific syntax error generated by a JSON parser;
  - `value` - the value set in the incoming request.

Example:

```
js>["adc1Raw", "adc2Raw", "js"]

{"adc1Raw":2107,"adc2Raw":2041,"js":{"error":{"edescr":"disabled!","val":""}}}
```
