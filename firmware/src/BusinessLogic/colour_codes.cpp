/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include <iostream>
using namespace std;
#include "colour_codes.h"


void set_board_colour(unsigned int* pCol_act, typeBoard nBoard)
{
    switch (nBoard)
    {
    case 1:
        for(int i = 0; i < 3; i++)
        {
        *(pCol_act + i) = col_DMS[i];
        }
      break;
    case 2:
        for(int i = 0; i < 3; i++)
        {
        *(pCol_act + i) = col_IEPE[i];
        }
        break;
    default:
        break;
    }
}


int main()
{

    set_board_colour(col_act, typeBoard::DMSBoard);

    cout << endl << endl;

    cout << "Die Boardfarbe ist: " << col_act[0] << " " << col_act[1] << " " << col_act[2] << endl;

    cout << endl;


    return 0;
}


