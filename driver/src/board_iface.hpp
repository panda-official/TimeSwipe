#include "bcmspi.h"
#include <iostream>
#include <thread>
#include <sstream>

static void stripAnswer(std::string& str) {
    if (str.length() && str[str.length()-1] == 0x0A) // strip \n
        str = str.substr(0,str.length()-1);
}

class BoardInterface
{
public:
    inline static bool trace_spi = false;

private:
    CBcmSPI spi;
    void sendCommand(const std::string& cmd) {
        CFIFO command;
        command += cmd;
        spi.send(command);
        if (trace_spi) {
            std::cerr << "spi: sent: \"" << command << "\"" << std::endl;
        }
    }
    bool receiveAnswer(std::string& ans) {
        CFIFO answer;
        if (spi.receive(answer)) {
            ans = answer;
            if (trace_spi) {
                std::cerr << "spi: received: \"" << ans << "\"" << std::endl;
            }
            return true;
        }
        if (trace_spi) {
            std::cerr << "spi: receive failed" << std::endl;
        }
        return false;
    }
    bool receiveAnswer(std::string& ans, std::string& error) {
        auto ret = receiveAnswer(ans);
        if (ret && !ans.empty() && ans[0]=='!') {
            error = ans;
            ans.clear();
            return false;
        }
        return ret;
    }
    bool receiveStripAnswer(std::string& ans, std::string& error) {
        if (!receiveAnswer(ans,error)) return false;
        stripAnswer(ans);
        return true;
    }
    bool receiveStripAnswer(std::string& ans) {
        std::string error;
        return receiveStripAnswer(ans, error);
    }
    void sendSetCommand(const std::string& variable, const std::string& value) {
        sendCommand(variable + "<" + value + "\n");
    }
    template <class NUMBER>
    bool sendSetCommandCheck(const std::string& variable, const NUMBER& value) {
        sendSetCommand(variable, std::to_string(value));
        // XXX: if sleep disable and trace_spi=false spi.receive fails sometimes
        //std::this_thread::sleep_for(std::chrono::nanoseconds(1));
        std::string answer;
        receiveStripAnswer(answer);
        NUMBER num;
        std::istringstream ss(answer);
        ss >> num;
        return num == value;
    }

    void sendGetCommand(const std::string& variable) {
        sendCommand(variable + ">\n");
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

    void setMode(int num) {
        sendSetCommand("Mode", std::to_string(num));
        std::string answer;
        //TODO: check answer
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

    bool setDAC(bool value) {
        sendSetCommand("DACsw", value ? "1" : "0");
        std::string answer;
        if (!receiveStripAnswer(answer)) return false;
        return answer == (value ? "1" : "0");
    }

    // num == 0 -> AOUT3  num == 1 -> AOUT4
    bool setOUT(uint8_t num, int val) {
        std::string var = std::string("AOUT") + (num?"4":"3") + ".raw";
        sendSetCommand(var, std::to_string(val));
        std::string answer;
        if (!receiveStripAnswer(answer)) return false;
        return answer == std::to_string(val);
    }

    // num == 0 -> PWM1  num == 1 -> PWM2
    bool startPWM(uint8_t num, uint32_t frequency, uint32_t high, uint32_t low, uint32_t repeats, float duty_cycle);

    // num == 0 -> PWM1  num == 1 -> PWM2
    bool stopPWM(uint8_t num) {
        std::string pwm = std::string("PWM") + std::to_string(num+1);
        /*
        sendGetCommand(pwm);
        std::string answer;
        receiveStripAnswer(answer);
        if (answer == "0") return false; // Already stopped
         */

        return sendSetCommandCheck(pwm, 0);
    }

    // num == 0 -> PWM1  num == 1 -> PWM2
    bool getPWM(uint8_t num, bool& active, uint32_t& frequency, uint32_t& high, uint32_t& low, uint32_t& repeats, float& duty_cycle);

};
