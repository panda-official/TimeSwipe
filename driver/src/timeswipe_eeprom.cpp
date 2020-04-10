#include "timeswipe_eeprom.hpp"
#include "HatsMemMan.h"
#include "defs.h"
#include <memory>
#include <fstream>

bool TimeSwipeEEPROM::Read(std::string& error) {
#if NOT_RPI
    return true;
#endif
    const char* i2c_file = "/sys/class/i2c-adapter/i2c-0/0-0050/eeprom";
    std::ifstream i2c(i2c_file);
    if (!i2c.is_open()) {
        //echo "24c32 0x50" > /sys/class/i2c-adapter/i2c-0/new_device
        std::ofstream ofs("/sys/class/i2c-adapter/i2c-0/new_device");
        if (!ofs.is_open()) {
            error = "Can not access i2c subsystem. Check drivers are properly loaded";
            return false;
        }
        std::string cmd_buf = "24c32 0x50\n";
        ofs.write(cmd_buf.c_str(), cmd_buf.length());
        ofs.close();
        if (!ofs) {
            error = "Create i2c failed. Check permissions";
            return false;
        }
        i2c.open(i2c_file, std::ifstream::in);
        if (!i2c.is_open()) {
            error = "Can not access i2c subsystem. Check drivers are properly loaded";
            return false;
        }
    }
    std::string str((std::istreambuf_iterator<char>(i2c)),
            std::istreambuf_iterator<char>());
    if (str.length() > 127) str.resize(127);
    auto buf = std::make_shared<CFIFO>();
    *buf += str;
    CHatsMemMan HatMan(buf);

    //The image has to be verified before use: Verify() methode will check the image consistency and CRCs of the atoms
    CHatsMemMan::op_result res;
    res = HatMan.Verify();
    if (res != CHatsMemMan::OK) {
        error = "EEPROM verify failed";
        return false;
    }

    //The number of atoms can be obtained as following (optional)
    unsigned int nAtoms = HatMan.GetAtomsCount();

    //Then obligatory atoms can be obtained:
    CHatAtomVendorInfo vi;
    CHatAtomGPIOmap gpio;

    res = HatMan.Load(vi);
    res = HatMan.Load(gpio);

    /*
    printf("uuid: %x-%x-%x-%x\n", vi.m_uuid[0], vi.m_uuid[1], vi.m_uuid[2], vi.m_uuid[3]);
    printf("pid: 0x%x\n", vi.m_PID);
    printf("vstr: %s\n", vi.m_vstr.c_str());
    printf("pstr: %s\n", vi.m_pstr.c_str());
    printf("GPIO:\n");
    for (int i = 0; i < 28; i++) {
        const auto& g = gpio.m_GPIO[i];
        printf("%d: func_sel: %u reserved: %u pulltype: %u is_used: %u\n", i + 2, g.func_sel, g.reserved, g.pulltype, g.is_used);
    }
     */

    return true;

}
