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
    bool receiveAnswer(std::string& ans, std::string& error) {
        auto ret = receiveAnswer(ans);
        if (ret && !ans.empty() && ans[0]=='!') {
            error = ans;
            ans.clear();
        }
        return ret;
    }
    void sendSetCommand(const std::string& variable, const std::string& value) {
        sendCommand(variable + "<" + value + "\n");
    }
    void sendEventsCommand() {
        sendCommand("je>\n");
    }
    void sendSetSettingsCommand(const std::string& request) {
        sendCommand("js<" + request + "\n");
    }
    void sendGetSettingsCommand(const std::string& request) {
        sendCommand("js>" + request + "\n");
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

    std::string getSetSettings(const std::string& request, std::string& error) {
        sendSetSettingsCommand(request);
        std::string answer;
        if (!receiveAnswer(answer, error)) {
            error = "read SPI failed";
        }
        return answer;
    }

    std::string getGetSettings(const std::string& request, std::string& error) {
        sendGetSettingsCommand(request);
        std::string answer;
        if (!receiveAnswer(answer, error)) {
            error = "read SPI failed";
        }
        return answer;
    }

};
