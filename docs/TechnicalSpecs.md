### Gain Settings
The 22 gain settings are defined by their firmware setting.
The combined Setting is the actual amplification, that is applied to the input signal.
1st gain stage is the output gain stage of either 1 or 1.375.
2nd gain stage is the input gain, which can be set between 0.125 and 128 in factors of 2.
3rd gain stage is a fixed gain of 1.8 introduced a butterworth filter in front of the ADC.
The combined gain can be set via JSON commands in firmware

1st gain stage | 2nd gain stage | fixed gain by filter | combined gain
---   | ---   | ---   | ---
1     | 0.125 | 1.8 | 0.225
1.375 | 0.125 | 1.8 | 0.310
1     | 0.25  | 1.8 | 0.450
1.375 | 0.25  | 1.8 | 0.619
1     | 0.5   | 1.8 | 0.900
1.375 | 0.5   | 1.8 | 1.238
1     | 1     | 1.8 | 1.800
1.375 | 1     | 1.8 | 2.475
1     | 2     | 1.8 | 3.600
1.375 | 2     | 1.8 | 4.950
1     | 4     | 1.8 | 7.200
1.375 | 4     | 1.8 | 9.900
1     | 8     | 1.8 | 14.400
1.375 | 8     | 1.8 | 19.800
1     | 16    | 1.8 | 28.800
1.375 | 16    | 1.8 | 39.600
1     | 32    | 1.8 | 57.600
1.375 | 32    | 1.8 | 79.200
1     | 64    | 1.8 | 115.200
1.375 | 64    | 1.8 | 158.400
1     | 128   | 1.8 | 230.400
1.375 | 128   | 1.8 | 316.800