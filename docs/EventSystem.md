# Event System

The firmware has a system for tracking user actions while one is interacting with the board's user interface: button and LED menus.
The events are stored as a table of named slots of the form {"name" : "value"} : each event has its unique text name. And the value can be boolean, integer or string type depending on the current event. When the current event table is requested from external communication interface it is cleared after readout.
If multiple events of the same name occured before being read  their value will be overwritten with the freshest one at the named slot.
Thus, there is always only one named slot per event in the table. And the table occupies fixed memory size.

Here is the table of all possible events of the current firmware:

<br />

Event name      |   Value type                      |   Description
--------------- | ----------------------------------|-------------------------------------------------------------------------------------
Button          |  boolean(true, false)             |   Notifies that user has pressed(true) or released(false) the button
ButtonStateCnt  |  integer(32bit)                   |   A counter of button state changes (odd value means the button is pressed, even means it is released)
Record          |  integer(32bit)                   |   Notifies that a new record stamp has been created (record count)
Gain            |  integer(32bit)                   |   Notifies the Gain setting has been changed
Bridge          |  boolean(true, false)             |   Notifies the Bridge setting has been changed
Mode            |  integer(32bit)                   |   Notifies the Mode setting has been changed ( 0 - IEPE, 1 - Normal Signal, 2 - Digital)
Offset          |  integer(32bit)                   |   Notifies that offset search routine has been started (1- negative offset search, 2- zero offset search, 3- positive offset search)

<br />

The current event table can be obtained via default communication protocol in a JSON format by "je>" command:

Request/Response               |  Command
------------------------------ | -------------------------------------------------------------------------------------------------------------------------
request message:               |  je>\n
successive response message:   |  { "Button" : true, "ButtonStateCnt" : 3 }\n - indicates the board's button was pressed and shows its state counter: odd value means the button is pressed, even means it is released.

Please, see CommunicationProtocol.md for default communication protocol details.


