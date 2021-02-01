#pragma once

//#define NOT_RPI !defined(__arm__) && !defined(__aarch64__)

#ifdef PANDA_BUILD_FIRMWARE_EMU
    #define NOT_RPI 1
#endif
