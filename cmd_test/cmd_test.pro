TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH+="../src/Abstracts"
INCLUDEPATH+="../src/Linux"
INCLUDEPATH+="../../../json/include"

SOURCES += \
        main.cpp \
    ../src/Abstracts/std_port.cpp \
    ../src/Abstracts/cmd.cpp \
    ../src/Abstracts/frm_stream.cpp \
    ../src/Abstracts/jsondisp.cpp \
    ../src/Abstracts/json_stream.cpp \
    ../src/Linux/console.cpp \
    ../src/Abstracts/json_evsys.cpp \
    ../src/Linux/OS_linux.cpp

HEADERS += \
    ../src/Abstracts/jsondisp.h \
    ../src/Abstracts/json_stream.h \
    ../src/Abstracts/json_evsys.h
