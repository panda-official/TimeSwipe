#include "bcmspi.h"

class BoardInterface
{
    CBcmSPI spi;
    void sendCommand(const std::string& cmd) {
        CFIFO command;
        command += cmd;
        spi.send(command);
    }
    bool receiveAnswer(std::string& ans) {
        CFIFO answer;
        if (spi.receive(answer)) {
            ans = answer;
            return true;
        }
        return false;
    }
    void sendSetCommand(const std::string& variable, const std::string& value) {
        sendCommand(variable + "<" + value + "\n");
    }
    void sendEventsCommand() {
        sendCommand("je>\n");
    }

    BoardInterface()
        : spi(CBcmLIB::iSPI::SPI0)
    {}

    static BoardInterface* _instance;
public:
    static BoardInterface* get() {
        if (!_instance) _instance = new BoardInterface();
        return _instance;
    }

    ~BoardInterface() {
        if (_instance) delete _instance;
    }

    void setBridge(int num) {
        sendSetCommand("Bridge", std::to_string(num));
        std::string answer;
        receiveAnswer(answer);
    }

    void setEnableADmes(bool value) {
        sendSetCommand("EnableADmes", value ? "1" : "0" );
        std::string answer;
        receiveAnswer(answer);
    }

    bool getEvents(std::string& ev) {
        sendEventsCommand();
        return receiveAnswer(ev);
    }
};
