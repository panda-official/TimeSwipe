/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

//SAM clocks contol:
#include <list>
#include <memory>

enum class typeSamCLK : int {none=-1,  MCLK=0, GCLK1, GCLK2, GCLK3, GCLK4, GCLK5, GCLK6, GCLK7, GCLK8, GCLK9, GCLK10, GCLK11};

class CSamCLK
{
//friend class std::shared_ptr<CSamCLK>;
protected:

    //clocks array:
    static std::list<CSamCLK *> m_Clocks;
    static bool m_bOcupied[12];

    int  m_nCLK;

    CSamCLK(){} //disable ctor,only factory

public:
    //static typeSamCLK GetFreeCLKind(); //dbg only version+++

    typeSamCLK CLKind(){return static_cast<typeSamCLK>(m_nCLK);}

    //factory:
    static std::shared_ptr<CSamCLK> Factory();
    ~CSamCLK();

    void WaitSync();
    void SetDiv(unsigned short div);
    void Enable(bool how);

};
