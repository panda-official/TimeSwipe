/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "SamCLK.h"

enum class typeSamTC : int {Tc0=0, Tc1, Tc2, Tc3, Tc4, Tc5, Tc6, Tc7};

class CSamTC
{
protected:
    typeSamTC m_nTC;

    void EnableAPBbus(typeSamTC nTC, bool how);

public:
    CSamTC(typeSamTC nTC);

    typeSamTC GetID(){return m_nTC;}

    void EnableIRQ(bool how);

    void EnableAPBbus(bool how){ EnableAPBbus(m_nTC, how); }

    void ConnectGCLK(typeSamCLK nCLK);
};

