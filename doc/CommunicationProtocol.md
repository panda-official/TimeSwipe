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

### Settings group `calibrationData`

These settings are used to control the board calibration data.

|Name                      |Description                       |Range|Access|Default|
|:-------------------------|:---------------------------------|:----|:-----|:------|
|calibrationData           |Calibration data                  |JSON |rw    |       |
|calibrationDataEnabled    |Calibration data enabled?         |bool |rw    |false  |
|calibrationDataApplyError |Calibration data last apply error |JSON |r     |       |
|calibrationDataEepromError|Calibration data last EEPROM error|JSON |r     |       |

#### Details

- `calibrationData` is available only on a calibration station. It's range:
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
|fanDutyCycle |Duty cycle (pulse width).|(0, 1)     |r     |       |
|fanFrequency |Frequency.               |[1,20000]  |rw    |100    |

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
|eepromTest        |Starts EEPROM test or gets the result.   |bool  |rw    |       |
|firmwareVersion   |Firmware version.                        |string|r     |       |
|temperature       |Core temperature of ARM chip.            |float |r     |       |
|uiTest            |Starts UI test or gets the result.       |bool  |rw    |       |
|uptime            |The uptime of the firmware in seconds.   |float |r     |       |

#### Details

- `eepromTest` is available only on a calibration station.
- `temperature` - value is in Celsius;
- `uiTest` is available only on a calibration station.

### Special settings

#### Special setting `all`

The special setting `all` provides a way to read or write multiple settings at
once.

##### Request format

To read multiple (all) settings at once no additional input required.

To write multiple settings at once they must be specified in a JSON object,
obviously, as: `{"setting":value, ...}`, where `setting` must not be a name of
any special setting.

##### Remarks

If writing of at least one setting is failed the entire operation is aborted
and error is returned. Therefore, if an error occurs upon attempt to modify
multiple settings at once, the values of some settings still remain unaffected!

#### Special setting `basic`

The special setting `basic` - is a subset of `all` settings that doesn't include
the `calibrationData`, `calibrationDataApplyError` and `calibrationDataEepromError`
settings.

##### Response format

The response will contain all the settings which were read or written.

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
`{"result": value}`, where `value` - is a JSON object;
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
{"result":{"channel1DacRaw":2048}}\n
```

#### 2. Reading actual adc2Raw value

##### Request

```
adc2Raw>\n
```

##### Response

```
{"result":{"adc2Raw":2048}}\n
```

#### 3. Update settings via `all` special command

##### Request

```
all<{"voltageOutEnabled":true,"channel1DacRaw":500,"channel2DacRaw":700,"channel3DacRaw":900,"channel4DacRaw":1100}\n
```

##### Response

```
{"result": {"voltageOutEnabled":true,"channel1DacRaw":500,"channel2DacRaw":700,"channel3DacRaw":900,"channel4DacRaw":1100}}\n
```

#### 4. Read all available settings (except specials)

##### Request

```
all>\n
```

##### Response

```
{"result": {<all the settings>}}\n
```
