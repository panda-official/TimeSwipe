/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

//Symmetric ADchn abstraction:

#pragma once

class CADchan{

protected:
        //data:
         float		m_k;			//scaling factor
         float		m_b;			//zero offs
         int		m_IntRange;		//ADCrange

          //real range:
          float m_RangeMin;
          float m_RangeMax;

          //data result:
          float m_RealVal;
          int   m_RawBinaryVal;

          //helpers: symmetric conversion:
          float RawBinary2Real(int RawVal)
          {
              if(RawVal<0)
                  RawVal=0;
              if(RawVal>m_IntRange)
                  RawVal=m_IntRange;

              return RawVal*m_k + m_b;
          }
          int   Real2RawBinary(float  RealVal)
          {
              int res=(int)((RealVal-m_b)/m_k);
              if(res<0)
                      return 0;
              if(res>m_IntRange)
                      return m_IntRange;
              return res;
          }

public:
          CADchan()
          {
              m_IntRange=1;
              m_RawBinaryVal=0;
              m_RealVal=0;
              SetRange(0, 1.0f);
          }
          virtual ~CADchan()=default;  //just to keep polymorphic behaviour, should be never called

          //methodes:
          float GetRealVal(){ return m_RealVal; }
          int GetRawBinVal(){ return m_RawBinaryVal; }

          void SetRealVal(float RealVal)
          {
              if(RealVal<m_RangeMin)
                  RealVal=m_RangeMin;
              if(RealVal>m_RangeMax)
                  RealVal=m_RangeMax;

              m_RealVal=RealVal;
              m_RawBinaryVal=Real2RawBinary(RealVal);
          }
          void SetRawBinVal(int RawVal)
          {
              m_RawBinaryVal=RawVal;
              m_RealVal=RawBinary2Real(RawVal);
          }

          void GetRange(float &min, float &max)
          {
                  min=m_RangeMin;
                  max=m_RangeMax;
          }
          void SetRange(float min, float max)
          {
              m_RangeMin=min;
              m_RangeMax=max;

              m_b=min;
              m_k=(max-min)/m_IntRange;
          }
};
