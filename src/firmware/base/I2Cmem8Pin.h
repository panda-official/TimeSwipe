/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#pragma once

#include "sam/SamI2Cmem.h"

/**
 * @brief A hardware-dependent implementation of CAT2430 EEPROM chip emulation
 * for external 8-Pin plug outputs.
 */
class CSamI2Cmem8Pin final : public CSamI2Cmem {
public:
  /// Setups PINs.
  CSamI2Cmem8Pin();
};
