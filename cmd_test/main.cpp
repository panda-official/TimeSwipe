/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include <iostream>

using namespace std;

//cmd unit test
#include "cmd.h"
#include "std_port.h"
#include "ADchan.h"
#include "Serial.h"
#include "jsondisp.h"
#include "console.h"

using json = nlohmann::json;
int main()
{
    auto pBus=          std::make_shared<CNixConsole>();
    auto pDisp=         std::make_shared<CCmdDispatcher>();
    auto pStdPort=      std::make_shared<CStdPort>(pDisp, pBus);
 //   int cnt=pStdPort.use_count();
    pBus->AdviseSink(pStdPort);
  //  cnt=pStdPort.use_count();

    auto pObj=          std::make_shared<CADchan>();
    pObj->SetRange(-10.0f, 10.0f);

    auto pObj2=          std::make_shared<CADchan>();
    pObj2->SetRange(0.0f, 4095.0f);
    pObj2->SetRealVal(2047.0f);

    pDisp->Add("DACA", std::make_shared< CCmdSGHandler<CADchan, float> >(pObj, &CADchan::GetRealVal, &CADchan::SetRealVal ) );

    pDisp->Add("ADC1", std::make_shared< CCmdSGHandler<CADchan, float> >(pObj, &CADchan::GetRealVal) );
    pDisp->Add("ADC2", std::make_shared< CCmdSGHandler<CADchan, float> >(pObj2, &CADchan::GetRealVal) );

    //07.06.2019: JSON test:
  /*  json jtest = {
      {"gain", 3},
      {"bridge", true},
      {"zero", {
        {"start", true}
      }},
      {"offsets", {
        {"DACA", 0.1},
        {"DACB", -1.2},
        {"DACC", 0.5},
        {"DACD", 1.6}
      }}
    };
    json resp;
    CJSONDispatcher jd(pDisp);
    jd.Call(jtest, resp, CCmdCallDescr::ctype::ctSet);
    std::cout<<resp<<std::endl;*/

    auto pJC=std::make_shared<CJSONDispatcher>(pDisp);
    pDisp->Add("js", pJC);

    CFIFO msg;
    while(1)
    {
        pBus->receive(msg);
    }

    return 0;
}
