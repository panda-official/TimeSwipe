# TimeSwipe Release Notes

## [Unreleased]

## [Changes][1.2.2] in version 1.2.2 relative to version 1.2.1

  - Added support for SAME53N19A chip;
  - Fixed the bug of TimeSwipe::Stop() which caused blocking of the working
  thread if the firmware not available;
  - Added `Uptime` SPI command which returns the uptime of firmware in seconds;
  - Reduced the chance of SPI timeout error by increasing max byte count in a
  silence frame.

## [Changes][1.2.1] in version 1.2.1 relative to version 0.1.1

  - Code improvements;
  - New function driver::version().

## [Changes][0.1.1] in version 0.1.1 relative to version 0.1.0

  - Bugfix of firmware

## [Changes][0.1.0] in version 0.1.0 relative to version 0.0.16

 - The voltage range calculation is simplifed to
 `(32768 Digits / 11.1 V) * Gain = XX Digits / V`;
 - The internal trigger buffer of PGA is not used now;
 - Driver headers are placed under `CMAKE_INSTALL_PREFIX/panda/timeswipe`, so
 the driver can be included with `#include <panda/timeswipe/driver.hpp>`
 directive after installation;
 - Only static library of the driver can be builded now;
 - Performance improvements of sensor data reading;
 - Fixed the problem with an invalid data at the beginning of each measurement;
 - New API to compensate a shift for very static measurements (aka *drift
 compensation* feature);
 - New `tool/plot.sh` to visualize the sensor data with [GNUPlot];
 - New command line tool `resampler` to resample CSV/binary files with sensor
 data;
 - New high performance resampler;
 - Inverted formula for `VSUP.raw`: `Raw = Real*k + b` (was `Real = Raw*k + b`);
 - New `tool/flash.expect` for a simpler firmware flashing with `expect(1)`;
 - New [Calibration Atom Specification](doc/CalibrationAtomSpecification.md);
 - New access point to control board's Fan output;
 - Reworked CMake scripts. Almost all CMake variables which affects the build
 now prefixed with `PANDA_`;
 - New `CONFIG_SCRIPT` section in `config.json` of `datlog` example for board
 configuration at start;
 - New API to set mode, gain and IEPE for each channel.

[Unreleased]: https://github.com/panda-official/timeswipe/compare/v1.2.2...HEAD
[1.2.2]: https://github.com/panda-official/timeswipe/compare/v1.2.1...v1.2.2
[1.2.1]: https://github.com/panda-official/timeswipe/compare/v0.1.1...v1.2.1
[0.1.1]: https://github.com/panda-official/timeswipe/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/panda-official/timeswipe/compare/v0.0.16...v0.1.0
[GNUPlot]: http://www.gnuplot.info
