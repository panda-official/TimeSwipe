/*!
 * \file
 * \brief this file is used only to generate the documentation
 *  
 * \page JSONstuff_page JSONstuff
 *
 * This directory brings communication extensions and some additional JSON-based functionality
 * from the nlohman::json library to the firmware
 *
 * CJSONbase is a superviser class used for controlling the entire JSON system. For exampe all JSON command handlers can be switched on/off
 * by calling a CJSONbase::LockCmdSubsys(...)
 *
 * CJSONStream - a class derived from CFrmStream provides a mechanism for retrieving/storing
 * primitive data types (int, float, std::string, e.t.c) from/to the JSON object in the CFrmStream style:
 * by extraction(>>) and insertion (<<) operators that allows easy integration to the communication system
 *
 * CJSONDispatcher - a handler for "js" command. <br />
 *
 * CJSONEvDispatcher - a handler for "je" command. <br />
 *
 * A simple event system where an event is a JSON object based on two classes: <br />
 *
 * IJSONEvent -a callback interface used to notify the derived class with JSON event
 * and  CJSONEvCP - a template for creating an event source with a connection point inside for the event subscribers.
 *
 */
 
 
