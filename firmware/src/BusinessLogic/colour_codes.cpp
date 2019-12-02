/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//#include <iostream>
//using namespace std;
#include "colour_codes.h"

 //   enum class typeBoard {DMSBoard, IEPEBoard};



    //Colour codes:

    unsigned int col_DMS[3] = {24, 250, 208};
    unsigned int col_IEPE[3] = {73, 199, 255};
    
    //unsigned int col_white[3] = {255, 255, 255};
    //unsigned int col_red[3] = {255, 0, 0};
    //actual colour:
    unsigned int col_act[3] = {0, 0, 0};


    //scalar codes:

    unsigned int col_DMS_sca = col_DMS[0]*65536 + col_DMS[1]*256 + col_DMS[2];
    unsigned int col_IEPE_sca = col_IEPE[0]*65536 + col_IEPE[1]*256 + col_IEPE[2];

    //unsigned int col_white_sca = col_white[0]*65536 + col_white[1]*256 + col_white[2];
    //unsigned int col_red_sca = col_red[0]*65536 + col_red[1]*256 + col_red[2];











