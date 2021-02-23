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
    unsigned int packetSize=1024;
    unsigned int packetMaxConsFails=5;

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

    std::string makeChCmd(unsigned int num, const char *pSubDomain){

        char buf[32];
        std::sprintf(buf, "CH%d.%s", num+1, pSubDomain);
        return buf;
    }

    bool setChannelMode(unsigned int num, int nMode){

        sendSetCommand(makeChCmd(num, "mode"), std::to_string(nMode));
        std::string answer;
        if (!receiveStripAnswer(answer)) return false;
        return answer == std::to_string(nMode);
    }
    bool getChannelMode(unsigned int num, int &nMode, std::string& error){

        sendGetCommand(makeChCmd(num, "mode"));
        std::string answer;
        if (!receiveAnswer(answer, error)) {
            nMode=0;
            return false;
        }
        nMode=std::stoi(answer);
        return true;

    }
    bool setChannelGain(unsigned int num, float Gain){

        sendSetCommand(makeChCmd(num, "gain"), std::to_string(Gain));
        std::string answer;
        if (!receiveStripAnswer(answer)) return false;
        return true;
    }
    bool getChannelGain(unsigned int num, float &Gain, std::string& error){

        sendGetCommand(makeChCmd(num, "gain"));
        std::string answer;
        if (!receiveAnswer(answer, error)) {
            Gain=0;
            return false;
        }
        Gain=std::stof(answer);
        return true;

    }
    bool setChannelIEPE(unsigned int num, bool bIEPE){

        sendSetCommand(makeChCmd(num, "iepe"), std::to_string(bIEPE));
        std::string answer;
        if (!receiveStripAnswer(answer)) return false;
        return true;
    }
    bool getChannelIEPE(unsigned int num, bool &bIEPE, std::string& error){

        sendGetCommand(makeChCmd(num, "iepe"));
        std::string answer;
        if (!receiveAnswer(answer, error)) {
            bIEPE=0;
            return false;
        }
        bIEPE=std::stoi(answer);
        return true;
    }

    //Measurement Iface:
    bool getMeasMask(uint8_t &mask, std::string& error){

        sendGetCommand("MeasChannel");
        std::string answer;
        if (!receiveAnswer(answer, error)) {
            mask=0;
            return false;
        }
        mask=std::stoi(answer);
        return true;
    }

    //file transfer:
    int readFPacket(const char *pFname, std::vector<uint8_t> &input, unsigned int pos, unsigned int count, std::string& error){

        CFIFO rbuf;
        std::string cmd=std::string(pFname)+"> "+std::to_string(pos)+" "+std::to_string(count)+"\n";

        sendCommand(cmd);
        if(!spi.receive(rbuf))
        {
            if (trace_spi) {
                std::cerr << "spi: failed to receive a packet" << std::endl;
            }
            return -1;
        }

        typeSChar ch;
        rbuf>>ch;
        if('!'==ch || 'f'!=ch)
        {
            error=rbuf;
            if (trace_spi) {
                std::cerr << error << std::endl;
            }
            return -1;
        }

        int cnt=rbuf.in_avail()-1; //exclude "\n"
        for(int i=0; i<cnt; i++)
        {
            rbuf>>ch;
            input.push_back(ch);
        }
        return cnt;
    }
    bool readFile(const char *pFname, std::vector<uint8_t>  &input, std::string& error)
    {
        input.clear();

        //read packets:
        int rv;
        int fail_cnt=0;
        do{

            rv=readFPacket(pFname, input, input.size(), packetSize, error);
            if(rv<0){

                fail_cnt++;
                if(fail_cnt>=packetMaxConsFails)
                    return false;
            }
            else {

                fail_cnt=0;
            }

        }while(0!=rv);

        return  true;
    }

};
