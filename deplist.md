# Dependencies and associated licences for the project:

## third party dependencies for the firmware:

- startup_ARMCM4.S: <br />
    assembler startup code <br />
    dir: /prj_templ/Custom <br />
    status: changed/tuned <br />
    license: ? (Please, see license conditions inside the file)
    
- gcc.ld, gcc_RAM.ld: <br />
    linker scripts <br />
    dir: /prj_templ/Custom <br />
    status: changed/tuned <br />
    license: ? (No license description)
    
- CMSIS library: <br />
    CortexMx control library <br />
    dir: /prj_templ/CMSIS <br />
    status: original <br />
    license: Apache-2.0 www.apache.org/licenses/LICENSE-2.0
    
- SAME54's header files from manufactor (Atmel/Microchip) <br />
    chip's perephireals description <br /> 
    dir: /prj_templ/include <br />
    status: original <br />
    license: Apache-2.0 www.apache.org/licenses/LICENSE-2.0
    
 - Adafruit_NeoPixel <br />
    an Arduino library used for control of LEDs <br />
    dir: /Adafruit_NeoPixel <br />
    status: changed/tuned <br />
    license: GNU LESSER GENERAL PUBLIC LICENSE
 
 -  nlohmann/json <br />
    JSON for Modern C++ https://nlohmann.github.io/json/ <br />
    dir: outside the project folder <br />
    status: original <br />
    license: MIT License
    
    ## third party dependencies for the rPIs client software:
    
 -  bcm2835 library  <br />
    C library for Broadcom BCM 2835 as used in Raspberry Pi  https://www.airspayce.com/mikem/bcm2835/ <br />
    dir: /BCMsrc <br />
    status: changed/tuned <br />
    license: Open Source Licensing GPL V2 https://www.gnu.org/licenses/gpl-2.0.html
    
