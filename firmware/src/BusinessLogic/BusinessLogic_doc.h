/*!
 * \file
 * \brief this file is used only to generate the documentation
 *  
 * \page BusinessLogic_page BusinessLogic
 *
 *  The basic functionality of the board is provided by the nodeControl class object.
 *  The only one controller object can exist thats why its designed as "singleton" class.
 *
 *  The following basic board functionality is implemented by a nodeControl:
 *
 *  nodeControl::SetGain(...)       - sets the board amplifier gain. <br />
 *  nodeControl::IncGain(...)       - increments the gain value (if the maximum gain is exideed the minimum vale will be set) <br />
 *  nodeControl::SetBridge(...)     - sets bridge voltage <br />
 *  nodeControl::SetOffset(...)     - starts/stops finding amplifier offsets procedure <br />
 *  nodeControl::StartRecord(...)   - sets a random 32-bit value as a new record stamp <br />
 *
 *  For detailed information please refer to the detailed class description - nodeControl <br />
 *
 *  The board's User Interface (UI) is physically implemented by a button and four colorized LEDs.
 *  By different LEDs behaviour and color combination the user "menus" are formed.
 *
 *  All possible views of the board are implemented in a separate View class:  CView with a view channel for each LED - CViewChannel
 *
 *  The menu logic is handled by CNewMenu class object. The class is designed in event-driven style and forced by events
 *  coming from a button ( SAMButton ). <br />
 *
 *  Another thing is a data visualization: displaying the measured signal levels by LEDs when user is not interacting with the board (default View mode).
 *  This is implemented by a CDataVis class
 */
 
 
