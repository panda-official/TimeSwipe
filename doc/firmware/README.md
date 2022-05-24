# PANDA Timeswipe Firmware

## API

The communication with the master device follows the *master-slave model*.
*Master* device sends a setting request and waiting for a response from a board.

The Panda Timeswipe Board can be controlled via SPI bus by sending the setting
requests and awaiting for responses as describes in the
[Firmware API](../firmware-api.md).
