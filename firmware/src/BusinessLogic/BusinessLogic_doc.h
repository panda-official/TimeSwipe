/*!
 * \file
 * \brief this file is used only to generate the documentation
 *  
 * \page BusinessLogic_page BusinessLogic
 *
 *  The basic functionality of the board is provided by the nodeControl class object.
 *  The object can be considered as somewhat usually called "controller" in MCV pattern for example.
 *  The only one controller object can exist thats why its designed as "singleton" class.
 *
 *  The following basic board functionality is implemented by a nodeControl:
 *
 *  nodeControl::SetGain(...)       - sets the board amplifier gain. <br />
 *  nodeControl::IncGain(...)       - increments the gain value (if the maximum gain is exideed the minimum vale will be set) <br />
 *  nodeControl::SetBridge(...)     - sets bridge voltage <br />
 *  nodeControl::SetZero(...)       - starts/stops finding amplifier offsets procedure <br />
 *  nodeControl::StartRecord(...)   - sets a random 32-bit value as a new record stamp <br />
 *
 *  For detailed information please refer to the detailed class description - nodeControl <br />
 *
 *  The board's User Interface (UI) is physically implemented by a button and four colorized LEDs.
 *  By different LEDs behaviour and color combination the user "menus" are formed.
 *  The menu logic is handled by CMenuLogic class object. The class is designed in event-driven style and forced by events
 *  coming from a button ( SAMButton ) and timer. <br />
 *
 *  Another thing is a data visualization: displaying the measured signal levels by LEDs when user is not interacting with the board.
 *  This is implemented by a CDataVis class
 */
 
 
