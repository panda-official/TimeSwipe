/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

/*class CFIFO : public std::vector<unsigned char>
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
};*/


//15.05.2019 : make FIFO from a stream bufer
/*#include <sstream>
class CFIFO : public std::stringbuf
{
public:
    void reset()    //reset all internals:
    {
        setp(pbase(), pbase());
        setg(eback(), eback(), eback());
        pubseekpos(0);
    }
    CFIFO & operator <<(typeSChar ch)
    {
        sputc(ch); return *this;
    }
    CFIFO & operator >>(typeSChar &ch)
    {
        ch=sbumpc(); return *this;
    }
};*/


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

//serial echo: software loopback for testing 10.05.2019
/*class CSerialEcho : public ISerialEvent
{
protected:
    std::shared_ptr<CSerial> m_pBus;

    virtual void on_rec_char(typeSChar ch)
    {
        m_pBus->send(ch);
    }
public:
    CSerialEcho(const std::shared_ptr<CSerial> &pBus) :m_pBus(pBus) {}
};*/

