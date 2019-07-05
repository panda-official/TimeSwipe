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
