import qbs

CppApplication {
    consoleApplication: true

    cpp.positionIndependentCode: false
    cpp.executableSuffix: ".elf"

    //property string rootPath: ".."
    property string rootPath: path.replace("/UTlight", "")
    property string jsonPath: "../../../json/include"

    files: [
        "../3rdparty/Adafruit_NeoPixel/Adafruit_NeoPixel.cpp",
        "../3rdparty/prj_templ/Custom/startup_ARMCM4.S",
        "../src/Abstracts/DAC.cpp",
        "../src/Abstracts/SyncCom.cpp",
        "../src/Abstracts/cmd.cpp",
        "../src/Abstracts/frm_stream.cpp",
        "../src/Abstracts/json_evsys.cpp",
        "../src/Abstracts/json_stream.cpp",
        "../src/Abstracts/jsondisp.cpp",
        "../src/Abstracts/std_port.cpp",
        "../src/BLogic/DataVis.cpp",
        "../src/BLogic/DataVis.h",
        "../src/BLogic/menu_logic.cpp",
        "../src/BLogic/nodeControl.cpp",
        "../src/Board/ADmux.cpp",
        "../src/Board/DACmax5715.cpp",
        "../src/Board/EventDisp.cpp",
        "../src/Board/MasterDetect.cpp",
        "../src/Board/OS_stub.cpp",
        "../src/Board/SAMbutton.cpp",
        "../src/Board/ino_stub_SAME54.cpp",
        "../src/Board/nodeLED.cpp",
        "../src/CortexMX/nodeTimeCM4.cpp",
        "../src/Procs/ADpointSearch.cpp",
        "../src/Procs/zerocal_man.cpp",
        "../src/SAMe54/SamADCcntr.cpp",
        "../src/SAMe54/SamCLK.cpp",
        "../src/SAMe54/SamDACcntr.cpp",
        "../src/SAMe54/SamQSPI.cpp",
        "../src/SAMe54/SamSPI.cpp",
        "../src/SAMe54/SamSPIsc2.cpp",
        "../src/SAMe54/SamSPIsc7.cpp",
        "../src/SAMe54/SamSercom.cpp",
        "../src/SAMe54/clock_set_SAME54.cpp",
        "main.cpp",
    ]
    Depends { name: "cpp" }

    cpp.defines: [
       // "__NO_SYSTEM_INIT",
        "__arm__",
        "__SAME54P20A__",
        "__SAMD51__",
        "SAM_BRM"

    ]

    cpp.includePaths: [

        rootPath+"/src/include",
        rootPath+"/src/Abstracts",
        rootPath+"/src/Board",
        rootPath+"/src/BLogic",
        rootPath+"/src/Procs",
        rootPath+"/src/SAMe54",
        rootPath+"/3rdparty/prj_templ/include",
        rootPath+"/3rdparty/prj_templ/CMSIS/Include",
        jsonPath

    ]

    //compiler:
    cpp.cxxFlags: [
        "-O0",
        "-std=c++17",

        "-mthumb",
        "-mcpu=cortex-m4",
        "-mfloat-abi=hard",
        "-mfpu=fpv4-sp-d16",

        "-ffunction-sections",
        "-fdata-sections",
      /*  "-fexceptions",
        "-exception",
        "-funwind-tables"*/
    ]

    cpp.driverFlags: [
        "-mthumb",
        "-mcpu=cortex-m4",
        "-mfloat-abi=hard",
        "-mfpu=fpv4-sp-d16",

        "-Xlinker",
        "--gc-sections",
        "-specs=nosys.specs",
            "-specs=nano.specs",
          //  "-u _printf_float",
        ]


    //linker:
    cpp.linkerFlags: [

            "-T"+rootPath+"/3rdparty/prj_templ/Custom/gcc_RAM.ld",
           // "-u _printf_float"
    ]
}
