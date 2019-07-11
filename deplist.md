# Dependencies and associated licences for the project:

## dependencies for the firmware:

- startup_ARMCM4.S :
    assembler startup code
    dir: /prj_templ/Custom
    status: changed/tuned 
    license: ARM (?, Please, see license conditions inside the file)
    
- CMSIS library:
    CortexMx control library
    dir: /prj_templ/CMSIS
    status: original
    license: Apache-2.0 www.apache.org/licenses/LICENSE-2.0
    
- SAME54's header files from manufactor (Atmel/Microchip)
    chip's perephireals description 
    dir: /prj_templ/include
    status: original
    license: Apache-2.0 www.apache.org/licenses/LICENSE-2.0
    
 - Adafruit_NeoPixel
    an Arduino library used for control of LEDs
    dir: Adafruit_NeoPixel
    status: changed/tuned
    license: GNU LESSER GENERAL PUBLIC LICENSE
 
 -  nlohmann/json
    JSON for Modern C++ https://nlohmann.github.io/json/
    dir: outside the project folder
    status: original
    license: MIT License
