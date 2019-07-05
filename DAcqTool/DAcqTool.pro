TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    ../src/rPI/bcmspi.cpp \
    ../BCMsrc/bcm2835.c \
    ../src/Abstracts/SyncCom.cpp \
    ../src/Abstracts/frm_stream.cpp \
    ../src/Linux/OS_linux.cpp

INCLUDEPATH+="../src/Abstracts"
INCLUDEPATH+="../src/Linux"
INCLUDEPATH+="../src/rPI"
INCLUDEPATH+="../BCMsrc"
