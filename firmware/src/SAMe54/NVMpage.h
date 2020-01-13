/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once


/*!
*   \file
*   \brief A definition file for NVMscpage struct: NVM Software Calibration Area Mapping
*
*/

/*!
 * \brief NVM Software Calibration Area Mapping
 *
 * \details "The NVM Software Calibration Area contains calibration data that are determined and written during
 *           production test. These calibration values should be read by the application software and written back to
 *           the corresponding register." manual, pages 59-60
 */

#include <stdint.h>
struct NVMscpage{

    //16-bit block1:
    uint16_t AC_BIAS            :2; //!<AC Comparator 0/1 Bias Scaling. To be written to the AC CALIB register.
    uint16_t ADC0_BIASCOMP      :3; //!<Bias comparator scaling. To be written to the ADC0 CALIB register.
    uint16_t ADC0_BIASREFBUF    :3; //!<Bias reference buffer scaling. To be written to the ADC0 CALIB register.
    uint16_t ADC0_BIASR2R       :3; //!<Bias rail-to-rail amplifier scaling. To be written to the ADC0 CALIB register.
    uint16_t res1               :5;

    //16-bit block2:
    uint16_t ADC1_BIASCOMP      :3; //!<Bias comparator scaling. To be written to the ADC1 CALIB register.
    uint16_t ADC1_BIASREFBUF    :3; //!<Bias reference buffer scaling. To be written to the ADC1 CALIB register.
    uint16_t ADC1_BIASR2R       :3; //!<Bias rail-to-rail amplifier scaling. To be written to the ADC1 CALIB register.
    uint16_t res2               :7;

    //16-bit block3:
    uint16_t USB_TRANSN         :5; //!<USB TRANSN calibration value. To be written to the USB PADCAL register.
    uint16_t USB_TRANSP         :5; //!<USB TRIM calibration value. To be written to the USB PADCAL register.
    uint16_t USB_TRIM           :3; //!<USB TRIM calibration value. To be written to the USB PADCAL register.
    uint16_t res3               :3;

    //!temperature block is started at 0x00800100 + 64 16-bit blocks from the begining=

    //+61 emty blocks:
    uint16_t gap[61];

    //----------------------4*32bit block end------------------------

    //32-bit blockT1:
    uint32_t TLI                :8; //!<Integer part of calibration temperature TL
    uint32_t TLD                :4; //!<Decimal part of calibration temperature TL
    uint32_t THI                :8; //!<Integer part of calibration temperature TH
    uint32_t THD                :4; //!<Decimal part of calibration temperature TH
    uint32_t res4               :8;

    //32-bit blockT2:
    uint32_t res5               :8;
    uint32_t VPL                :12; //!<Temperature calibration parameter1
    uint32_t VPH                :12; //!<Temperature calibration parameter2

    //32-bit blockT3:
    uint32_t VCL                :12; //!<Temperature calibration parameter3
    uint32_t VCH                :12; //!<Temperature calibration parameter4
};


