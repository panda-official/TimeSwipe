/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   @file
*   @brief A definition file for Analog-to-Digital or Digital-to-Analog measurement/control channel
*   CADchan
*
*/

#pragma once


/*!
 * \brief An Analog-Digital channel class
 *
 * \details ADC and DAC devices usually contain a number of measurement/controlling units called channels.
 * This class describes basic ADC/DAC channel functionality:
 *
 * 1) storing a measured/control value in a real units: Volts, A/mA and so on
 * 2) storing the range of the channel in a real units: for example -10 +10 Volts
 * 3) realising convertion from the real value to raw-binary value native for ADC/DAC chip or board and backword convertion
 * 4) storing convertion factors: k & b
 */
class CADchan{

protected:

         /*!
          * \brief Proportional convertion factor k: RealValue=RawValue*k + b
          */
         float		m_k;

         /*!
          * \brief Zero offset: RealValue=RawValue*k + b
          */
         float		m_b;

         /*!
          * \brief  The range of the chip in discrets(raw-binary fromat)
          */
         int		m_IntRange;		//ADCrange


         /*!
          * \brief The minimum range of the channel in real units (V, a, mA...)
          */
         float m_RangeMin;

         /*!
          * \brief The maximum range of the channel in real units (V, a, mA...)
          */
         float m_RangeMax;

         /*!
          * \brief An actual value of the hannel in real units
          */
         float m_RealVal;

         /*!
          * \brief An actual value ot the channel in the raw-binary format(native chip format)
          */
         int   m_RawBinaryVal;


         /*!
          * \brief Transformation from raw-binary value to real units value
          * \param RawVal The value in a raw-binary format
          * \return Real value in defined units
          */
          float RawBinary2Real(int RawVal)
          {
              if(RawVal<0)
                  RawVal=0;
              if(RawVal>m_IntRange)
                  RawVal=m_IntRange;

              return RawVal*m_k + m_b;
          }

          /*!
           * \brief Transformation from real value to raw-binary format (native for the chip)
           * \param RealVal The value in a real units
           * \return Raw-binary value
           */
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
          /*!
           * \brief Class constructor
           */
          CADchan()
          {
              m_IntRange=1;
              m_RawBinaryVal=0;
              m_RealVal=0;
              SetRange(0, 1.0f);
          }
          //! virtual destructor
          virtual ~CADchan()=default;  //just to keep polymorphic behaviour, should be never called

          //methodes:
          /*!
           * \brief An actual measured/controlled value in real units
           * \return A value in real units
           */
          float GetRealVal(){ return m_RealVal; }

          /*!
           * \brief An actual measured/controlled value in raw-binary format
           * \return A value in real units
           */
          int GetRawBinVal(){ return m_RawBinaryVal; }

          /*!
           * \brief Set the actual measured/controlled value in real units, fits the value according set range
           * \param RealVal the value
           */
          void SetRealVal(float RealVal)
          {
              if(RealVal<m_RangeMin)
                  RealVal=m_RangeMin;
              if(RealVal>m_RangeMax)
                  RealVal=m_RangeMax;

              m_RealVal=RealVal;
              m_RawBinaryVal=Real2RawBinary(RealVal);
          }

          /*!
           * \brief Set the actual measured/controlled value in raw-binary format
           * \param RealVal the value
           */
          void SetRawBinVal(int RawVal)
          {
              m_RawBinaryVal=RawVal;
              m_RealVal=RawBinary2Real(RawVal);
          }

          /*!
           * \brief Get the real value range
           * \param min Minimum range
           * \param max Maximum range
           */
          void GetRange(float &min, float &max)
          {
                  min=m_RangeMin;
                  max=m_RangeMax;
          }

          /*!
           * \brief Set the real value range
           * \param min Minimum range
           * \param max Maximum range
           */
          void SetRange(float min, float max)
          {
              m_RangeMin=min;
              m_RangeMax=max;

              m_b=min;
              m_k=(max-min)/m_IntRange;
          }

          inline void SetLinearFactors(float k, float b)
          {
              m_k=k;
              m_b=b;
          }
};
