/**
 * \file
 *
 * \brief Component description for TCC
 *
 * Copyright (c) 2018 Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#ifndef _SAME54_TCC_COMPONENT_
#define _SAME54_TCC_COMPONENT_

/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR TCC */
/* ========================================================================== */
/** \addtogroup SAME54_TCC Timer Counter Control */
/*@{*/

#define TCC_U2213
#define REV_TCC                     0x310

/* -------- TCC_CTRLA : (TCC Offset: 0x00) (R/W 32) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t :3;               /*!< bit:  2.. 4  Reserved                           */
    uint32_t RESOLUTION:2;     /*!< bit:  5.. 6  Enhanced Resolution                */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t PRESCALER:3;      /*!< bit:  8..10  Prescaler                          */
    uint32_t RUNSTDBY:1;       /*!< bit:     11  Run in Standby                     */
    uint32_t PRESCSYNC:2;      /*!< bit: 12..13  Prescaler and Counter Synchronization Selection */
    uint32_t ALOCK:1;          /*!< bit:     14  Auto Lock                          */
    uint32_t MSYNC:1;          /*!< bit:     15  Master Synchronization (only for TCC Slave Instance) */
    uint32_t :7;               /*!< bit: 16..22  Reserved                           */
    uint32_t DMAOS:1;          /*!< bit:     23  DMA One-shot Trigger Mode          */
    uint32_t CPTEN0:1;         /*!< bit:     24  Capture Channel 0 Enable           */
    uint32_t CPTEN1:1;         /*!< bit:     25  Capture Channel 1 Enable           */
    uint32_t CPTEN2:1;         /*!< bit:     26  Capture Channel 2 Enable           */
    uint32_t CPTEN3:1;         /*!< bit:     27  Capture Channel 3 Enable           */
    uint32_t CPTEN4:1;         /*!< bit:     28  Capture Channel 4 Enable           */
    uint32_t CPTEN5:1;         /*!< bit:     29  Capture Channel 5 Enable           */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :24;              /*!< bit:  0..23  Reserved                           */
    uint32_t CPTEN:6;          /*!< bit: 24..29  Capture Channel x Enable           */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TCC_CTRLA_OFFSET            0x00         /**< \brief (TCC_CTRLA offset) Control A */
#define TCC_CTRLA_RESETVALUE        _U_(0x00000000) /**< \brief (TCC_CTRLA reset_value) Control A */

#define TCC_CTRLA_SWRST_Pos         0            /**< \brief (TCC_CTRLA) Software Reset */
#define TCC_CTRLA_SWRST             (_U_(0x1) << TCC_CTRLA_SWRST_Pos)
#define TCC_CTRLA_ENABLE_Po