/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//#include <iostream>
//using namespace std;
#include "board_type.h"
#include "nodeLED.h"

 //   enum class typeBoard {DMSBoard, IEPEBoard};

    typeBoard m_Board = typeBoard::IEPEBoard;

    //Colour codes:

    unsigned int col_DMS[3] = {24, 250, 208};
    unsigned int col_IEPE[3] = {73, 199, 255};
    
    unsigned int DMS_COLOR = LEDrgb(col_DMS[0], col_DMS[1], col_DMS[2]);
    unsigned int IEPE_COLOR = LEDrgb(col_IEPE[0], col_IEPE[1], col_IEPE[2]);


    //actual board colour:
    unsigned int col_act[3] = {0, 0, 0};


   














