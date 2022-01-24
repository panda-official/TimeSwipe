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

### Settings group `calibrationData`

These settings are used to control the board calibration data.

|Name                  |Description              |Range|Access|Default|
|:---------------------|:------------------------|:----|:-----|:------|
|calibrationData       |Calibration data         |JSON |rw    |       |
|calibrationDataEnabled|Calibration data enabled?|bool |rw    |false  |
|calibrationDataValid  |Calibration data valid?  |bool |r     |       |

#### Details

- `calibrationData` range:
  - for read access: empty, positive integer or array of positive integers;
  - for write access: JSON array of the following layout:
  ```
  [{"type":%, "data":[{"slope":%, "offset":%},...]},...]
  ```
  where `type` value is a calibration atom type, `slope` value is a float,
  `offset` is a signed integer.

### Settings group `channel`

These settings are used to control board channels.

|Name              |Description                 |Range    |Access|Default|
|:-----------------|:---------------------------|:--------|:-----|:------|
|channel%AdcRaw    |ADC measured value.         |[0,4095] |r     |       |
|channel%DacRaw    |Offset on the input signal. |[0,4095] |rw    |2048   |
|channel%Mode      |Measurement mode.           |[0,1]    |rw    |0      |
|channel%Gain      |Gain value.                 |[1, 1408]|rw    |1      |
|channel%Iepe      |IEPE?                       |bool     |rw    |false  |
|channelsAdcEnabled|ADC measurement enabled?    |bool     |rw    |false  |

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

#### Special setting `all`

The special setting `all` provides a way to read or write multiple settings at
once.

##### Request format

To read multiple (all) settings at once no additional input required.

To write multiple settings at once they must be specified in a JSON object,
obviously, as: `{"setting":value, ...}`, where `setting` must not be a name of
any special setting.

#### Special setting `basic`

The special setting `basic` - is a subset of `all` settings that doesn't include
the `calibrationData` setting.

##### Response format

The response will contain all the settings which were read or written.

#### Special setting `je`

The result of reading this setting - is a JSON object with the latest events.

## Communication protocol

### Setting request

The syntax of the setting request is the following (note, the real request must
not contain spaces inbetween `setting`, `type` and `value`):

```
name type input\n
```

where

- `name` - a setting name. Must be alpha-numeric identifier;
- `type` - a request type. Must be `>` (read) or `<` (write);
- `input` - (optional) - must be a valid JSON value.

### Setting response

There are two types of responses:

- result of successful request - is a JSON object of the following structure:
`{"result": value}`, where `value` may be an any JSON value;
- result of failed request - is a JSON object of the following structure:
`{"error": code, "what": "message"}`, where `code` - is a positive integer,
`message` - is a JSON string with the error explanation string.

### Examples

#### 1. Setting an offset value for channel 1 to 2048 discrets

##### Request

```
channel1DacRaw<2048\n
```

##### Response

```
{"result":2048}\n
```

#### 2. Preset a value for analog output 3 to 2048 discrets to be controlled
in the manual mode

##### Request

```
analogOut3Raw<2048\n
```

##### Response

```
{"result":2048}\n
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
{"result":true}\n
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
{"result":3000}\n
```

#### 5. Reading actual adc2Raw value

##### Request

```
adc2Raw>\n
```

##### Response

```
{"result":2048}\n
```

#### 6. Update settings via `all` special command

##### Request

```
all<{"Gain":3,"voltageOutEnabled":true,"channel1DacRaw":500,"channel2DacRaw":700,"channel3DacRaw":900,"channel4DacRaw":1100}\n
```

##### Response

```
{"result": {"Gain":3,"voltageOutEnabled":true,"channel1DacRaw":500,"channel2DacRaw":700,"channel3DacRaw":900,"channel4DacRaw":1100}}\n
```

#### 7. Read all available settings (except specials)

##### Request

```
all>\n
```

##### Response

```
{"result": {<all the settings>}}\n
```
