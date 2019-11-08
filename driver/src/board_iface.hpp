#include "bcmspi.h"

class BoardInterface
{
    CBcmSPI spi;
    void sendCommand(const std::string& cmd) {
        CFIFO command;
        command += cmd;
        spi.send(command);
    }
    void sendSetCommand(const std::string& variable, const std::string& value) {
        sendCommand(variable + "<" + value + "\n");
    }
public:
    BoardInterface()
        : spi(CBcmLIB::iSPI::SPI0)
    {}

    void setBridge(int num) {
        sendSetCommand("Bridge", std::to_string(num));
    }

    void setEnableADmes(bool value) {
        sendSetCommand("EnableADmes", value ? "1" : "0" );
    }
};