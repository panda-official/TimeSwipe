/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//SAM's sercoms enumeration:
#pragma once

enum class typeSamSercoms : int {Sercom0=0, Sercom1, Sercom2, Sercom3, Sercom4, Sercom5, Sercom6, Sercom7};

//24.06.2019:
enum class typeSamSercomIRQs : int {IRQ0=0, IRQ1, IRQ2, IRQ3};

#include "Serial.h"
#include "SamCLK.h"

extern "C"{
void SERCOM0_0_Handler(void);
void SERCOM0_1_Handler(void);
void SERCOM0_2_Handler(void);
void SERCOM0_3_Handler(void);
void SERCOM1_0_Handler(void);
void SERCOM1_1_Handler(void);
void SERCOM1_2_Handler(void);
void SERCOM1_3_Handler(void);
void SERCOM2_0_Handler(void);
void SERCOM2_1_Handler(void);
void SERCOM2_2_Handler(void);
void SERCOM2_3_Handler(void);
void SERCOM3_0_Handler(void);
void SERCOM3_1_Handler(void);
void SERCOM3_2_Handler(void);
void SERCOM3_3_Handler(void);
void SERCOM4_0_Handler(void);
void SERCOM4_1_Handler(void);
void SERCOM4_2_Handler(void);
void SERCOM4_3_Handler(void);
void SERCOM5_0_Handler(void);
void SERCOM5_1_Handler(void);
void SERCOM5_2_Handler(void);
void SERCOM5_3_Handler(void);
void SERCOM6_0_Handler(void);
void SERCOM6_1_Handler(void);
void SERCOM6_2_Handler(void);
void SERCOM6_3_Handler(void);
void SERCOM7_0_Handler(void);
void SERCOM7_1_Handler(void);
void SERCOM7_2_Handler(void);
void SERCOM7_3_Handler(void);
}

class CSamSercom : virtual public CSerial
{
protected:
    typeSamSercoms m_nSercom; //24.06.2019

protected:
    //ctor/dtor: 24.06.2019:
    CSamSercom(typeSamSercoms nSercom);
    virtual ~CSamSercom();

    //IRQs:
    virtual void OnIRQ0(){}
    virtual void OnIRQ1(){}
    virtual void OnIRQ2(){}
    virtual void OnIRQ3(){}
    void EnableIRQ(typeSamSercomIRQs nLine, bool how);

    static void EnableSercomBus(typeSamSercoms nSercom, bool how);
    static void ConnectGCLK(typeSamSercoms nSercom, typeSamCLK nCLK);
   // static void SetupIRQhandlers(typeSamSercoms nSercom, void (*pIRQ1)(void), void (*pIRQ2)(void), void (*pIRQ3)(void), void (*pIRQ4)(void));

//friends:
friend void SERCOM0_0_Handler(void);
friend void SERCOM0_1_Handler(void);
friend void SERCOM0_2_Handler(void);
friend void SERCOM0_3_Handler(void);

friend void SERCOM1_0_Handler(void);
friend void SERCOM1_1_Handler(void);
friend void SERCOM1_2_Handler(void);
friend void SERCOM1_3_Handler(void);

friend void SERCOM2_0_Handler(void);
friend void SERCOM2_1_Handler(void);
friend void SERCOM2_2_Handler(void);
friend void SERCOM2_3_Handler(void);

friend void SERCOM3_0_Handler(void);
friend void SERCOM3_1_Handler(void);
friend void SERCOM3_2_Handler(void);
friend void SERCOM3_3_Handler(void);

friend void SERCOM4_0_Handler(void);
friend void SERCOM4_1_Handler(void);
friend void SERCOM4_2_Handler(void);
friend void SERCOM4_3_Handler(void);

friend void SERCOM5_0_Handler(void);
friend void SERCOM5_1_Handler(void);
friend void SERCOM5_2_Handler(void);
friend void SERCOM5_3_Handler(void);

friend void SERCOM6_0_Handler(void);
friend void SERCOM6_1_Handler(void);
friend void SERCOM6_2_Handler(void);
friend void SERCOM6_3_Handler(void);

friend void SERCOM7_0_Handler(void);
friend void SERCOM7_1_Handler(void);
friend void SERCOM7_2_Handler(void);
friend void SERCOM7_3_Handler(void);
};
