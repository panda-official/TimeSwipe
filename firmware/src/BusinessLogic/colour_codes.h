/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#ifndef COLOUR_CODES_H
#define COLOUR_CODES_H

    enum class typeBoard:int {DMSBoard=1, IEPEBoard};

//Das mit der enum class funktionert noch nicht. Drüber lesen!

    //Colour codes:

    unsigned int col_DMS[3] = {24, 250, 208};
    unsigned int col_IEPE[3] = {50, 151, 247};


    //actual colour:

    unsigned int col_act[3] = {0, 0, 0};
//    unsigned int* pcol_act = &col_act[0];


    void set_board_colour(unsigned int* pCol_act, typeBoard nBoard);

//Über const pointer lesen! Der pointer wird nie woanders hin zeigen als zur Adresse von col_act.
    

#endif // COLOUR_CODES_H
