/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//serial device base structures and interfaces:

#pragma once

//#include <queue>
//#include <list>
#include <vector>
#include <memory>

//typedef unsigned char typeSChar;
typedef int typeSChar;

//fifo bufer:
//string-based:
class CFIFO : public std::string
{
protected:
        int m_ReadInd=0;

public:
        CFIFO & operator <<(typeSChar b)
        {
                push_back(b);
                return *this;
        }
        CFIFO & operator >>(typeSChar &b)
        {
                b=at(m_ReadInd++);
                return *this;
        }
        int in_avail() const { return size()-m_ReadInd; }
        void reset() { clear(); m_ReadInd=0; }
        void rewind(){ m_ReadInd=0; } //17.06.2019
};


//basic serial interface:
struct ISerial
{
	virtual bool send(CFIFO &msg)=0;
	virtual bool receive(CFIFO &msg)=0;

        virtual bool send(typeSChar ch)=0;
        virtual bool receive(typeSChar &ch)=0;

        //forbid coping:
        ISerial()=default;
        ISerial(const ISerial&) = delete;
        ISerial& operator=(const ISerial&) = delete;

protected:
         virtual ~ISerial()=default;

};

//events:
struct ISerialEvent
{
        virtual void on_rec_char(typeSChar ch)=0;

        //forbid coping:
        ISerialEvent()=default;
        ISerialEvent(const ISerialEvent&) = delete;
        ISerialEvent& operator=(const ISerialEvent&) = delete;

protected:
        virtual ~ISerialEvent()=default;

};

//template for a serial int:
class CSerial : public virtual ISerial
{
protected:
        std::vector< std::weak_ptr<ISerialEvent> > m_EvSinks;

        void Fire_on_rec_char(typeSChar ch)
        {
            for(std::vector< std::weak_ptr<ISerialEvent> >::const_iterator i=m_EvSinks.begin(); i!=m_EvSinks.end(); i++)
            {
                if(i->expired())
                {
                   m_EvSinks.erase(i);
                }
                else
                {
                    i->lock()->on_rec_char(ch);
                }
            }
        }
public:
        void AdviseSink(const std::shared_ptr<ISerialEvent> &sink)
        {
            m_EvSinks.emplace_back(sink);
        }

protected:
        virtual ~CSerial()=default;
};

