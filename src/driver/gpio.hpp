// -*- C++ -*-

// PANDA TimeSwipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef PANDA_TIMESWIPE_GPIO_HPP
#define PANDA_TIMESWIPE_GPIO_HPP

// I/O access
extern volatile unsigned int *gpio;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define PANDA_TIMESWIPE_INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define PANDA_TIMESWIPE_OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define PANDA_TIMESWIPE_SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define PANDA_TIMESWIPE_GPIO_SET *(gpio+7)  // sets bits which are 1, ignores bits which are 0
#define PANDA_TIMESWIPE_GPIO_CLR *(gpio+10) // clears bits which are 1, ignores bits which are 0

#define PANDA_TIMESWIPE_GET_GPIO(g) (*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define PANDA_TIMESWIPE_GPIO_PULL *(gpio+37) // Pull up/pull down
#define PANDA_TIMESWIPE_GPIO_PULLCLK0 *(gpio+38) // Pull up/pull down clock

//
// Set up a memory regions to access GPIO
//
void setup_io();

#endif  // PANDA_TIMESWIPE_GPIO_HPP
