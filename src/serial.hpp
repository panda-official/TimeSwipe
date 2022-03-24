// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/**
 * @file
 * Basic serial data types and interfaces.
 */

#ifndef PANDA_TIMESWIPE_SERIAL_HPP
#define PANDA_TIMESWIPE_SERIAL_HPP

#include <memory>
#include <string>
#include <vector>

/// Character type.
using Character = int;

/*!
 * \brief A First-In-First-Out buffer implementation.
 *
 * \details The FIFO buffer is used as basic data storage/exchange primitive type in the firmware.
 * Derived from std::string it also inherits std::string and std::vector functionality
 *
 * \todo Maybe it was not really good idea to inherit std::string and std::vector in a pure FIFO buffer
 * but it is often required a functionality of a memory buffer with a random access.
 * any ideas?
 */

class CFIFO : public std::string
{
protected:
    size_type m_ReadInd=0;

public:
    /// The default constructor.
    CFIFO() = default;

    /// The constructor.
    explicit CFIFO(std::string input) noexcept
      : std::string{std::move(input)}
    {}

        /*!
         * \brief insertion operator
         * \param b a simbol to be inserted according to FIFO order
         * \return reference to this
         */
        CFIFO & operator <<(Character b)
        {
                push_back(b);
                return *this;
        }

        /*!
         * \brief extraction operator
         * \param b a simbol to be extracted according to FIFO order
         * \return reference to this
         */

        CFIFO & operator >>(Character &b)
        {
                b=at(m_ReadInd++);
                return *this;
        }

        /*!
         * \brief how many elements are available in the FIFO buffer?
         * \return the number of available elements.
         */
        size_type in_avail() const { return size()-m_ReadInd; }

        /*!
         * \brief remove all elements from the buffer
         */
        void reset() { clear(); m_ReadInd=0; }

        /*!
         * \brief restore all elements that have been exctracted form the bufer by >> extraction operator
         */
        void rewind(){ m_ReadInd=0; }
};


/*!
 * \brief Light&Fast FIFO buffer implementation.
 *
 * \details This special FIFO buffer is designed for use in IRQ routines
 *
 */
template<int nBufSize>
class CFIFOlt
{
protected:
        int m_ReadInd=0;
        int m_WriteInd=0;
        char *m_pBuf;

        char m_Buf[nBufSize];

public:
        CFIFOlt()
        {
            m_pBuf=m_Buf;
        }

        /*!
         * \brief insertion operator
         * \param b a simbol to be inserted according to FIFO order
         * \return reference to this
         */
        CFIFOlt& operator<<(Character b)
        {
                if(m_WriteInd>=nBufSize)
                {
                    m_WriteInd=0;
                }
                m_pBuf[m_WriteInd++]=b;
                return *this;
        }

        /*!
         * \brief extraction operator
         * \param b a simbol to be extracted according to FIFO order
         * \return reference to this
         */

        CFIFOlt& operator>>(Character &b)
        {
                b=m_pBuf[m_ReadInd++];
                return *this;
        }

        /*!
         * \brief Dumps content of this buffer to another and resets this buffer
         * \details The operation is used to pass received data from IRQ routine to a normal thread (another FIFO) where
         *  processing speed is not critical and immideatly free this buffer for receiving a new incoming data
         * \param dest A buffer to pass the received data
         */
        void dumpres(CFIFOlt& dest)
        {
            char *pNewBuf=dest.m_pBuf;
            dest.m_pBuf=m_pBuf;
            m_pBuf=pNewBuf;
            dest.m_ReadInd=m_ReadInd;
            dest.m_WriteInd=m_WriteInd;
            m_ReadInd=0;
            m_WriteInd=0;
        }

        /*!
         * \brief how many elements are available in the FIFO buffer?
         * \return the number of available elements.
         */
        int in_avail() const { return m_WriteInd-m_ReadInd; }

        /*!
         * \brief remove all elements from the buffer
         */
        void reset() { m_WriteInd=0; m_ReadInd=0; }

        /*!
         * \brief restore all elements that have been exctracted form the bufer by >> extraction operator
         */
        void rewind(){ m_ReadInd=0; }
};


/*!
 * \brief A basic serial communication interface
 *
 * \details The interface allows derived classes to communicate by exchanging serial messages
 *  (character sequences) which are stored in FIFO buffers. This is base interface class for implementing a serial device.
 *
 */
struct ISerial
{
    /*!
     * \brief send a serial message to this class object
     * \param msg  a message to send (output parameter)
     * \return operation result: true if successful otherwise - false
     */
    virtual bool send(CFIFO &msg)=0;

    /*!
     * \brief receive a serial message from this class object
     * \param msg a message to receive (input parameter)
     * \return operation result: true if successful otherwise - false
     */
    virtual bool receive(CFIFO &msg)=0;


     //! default constructor
     ISerial()=default;

     /*!
      * \brief ISerial remove copy constructor
      * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
      *  that is unacceptable)
      */
     ISerial(const ISerial&) = delete;

     /*!
      * \brief operator = remove copy operator
      * \return
      * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
      *  that is unacceptable)
      */
     ISerial& operator=(const ISerial&) = delete;

protected:
     //! virtual destructor
     virtual ~ISerial()=default;

};

/**
 * @brief A callback interface used to notify the derived classes about some
 * event emitted by a serial device.
 */
class Serial_event_handler {
public:
  /// The destructor.
  virtual ~Serial_event_handler() = default;

  /**
   * @brief Callback that is called when a character `ch` has been received in
   * a FIFO buffer of a serial device.
   */
  virtual void handle_receive(Character ch) = 0;
};


/*!
 * \brief   A basic class for all serial devices
 * \details This is a template for deriving all serial devices.
 *  It implements a connection point for Serial_event_handler inside. All objects that realize Serial_event_handler can be advised
 *  to this serial device by AdviseSink and receive corresponding notifications
 *
 */
class CSerial : public virtual ISerial
{
protected:

        /*!
         * \brief A list of connection points for Serial_event_handler
         */
        std::vector<std::weak_ptr<Serial_event_handler>> m_EvSinks;

        /*!
         * \brief Notify all connected objects with
         *  "a new character has been received in a FIFO buffer of a serial device" event
         * \param ch a character that has been recived
         */
         void Fire_on_rec_char(const Character ch)
         {
           for (auto i = cbegin(m_EvSinks), e = cend(m_EvSinks); i != e;) {
             if (i->expired()) {
               i = m_EvSinks.erase(i);
               e = cend(m_EvSinks);
             } else {
               i->lock()->handle_receive(ch);
               ++i;
             }
           }
         }
public:

        /*!
         * \brief Subscribe a new listener to serial device events
         * \param A sink to an object to subscribe
         */
        void AdviseSink(const std::shared_ptr<Serial_event_handler> &sink)
        {
            m_EvSinks.emplace_back(sink);
        }

protected:

        //! virtual destructor
        virtual ~CSerial()=default;
};

#endif  // PANDA_TIMESWIPE_SERIAL_HPP
