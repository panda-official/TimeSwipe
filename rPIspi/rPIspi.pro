TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    ../BCMsrc/bcm2835.c \
    ../src/Abstracts/SyncCom.cpp \
    ../src/Abstracts/frm_stream.cpp \
    ../src/Linux/OS_linux.cpp \
    ../src/Linux/console.cpp \
    ../src/rPI/bcmspi.cpp \
    ../src/rPI/BSC_SLV_SPI.cpp

HEADERS += \
    ../src/Linux/nixconsole.h \
    ../BCMsrc/BCS_SLV_SPI.h


INCLUDEPATH+="../src/Abstracts"
INCLUDEPATH+="../src/Linux"
INCLUDEPATH+="../src/rPI"
INCLUDEPATH+="../BCMsrc"
