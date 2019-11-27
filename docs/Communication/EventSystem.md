# Event System

The firmware has a system for tracking user actions while one is interacting with the board's user interface: button and LED menus.
The events are stored as a table of named slots of the form {"name" : "value"} : each event has its unique text name. And the value can be boolean, integer or string type depending on the current event. When the current event table is requested from external communication interface it is cleared after readout.
If multiple events of the same name occured before being read  their value will be overwritten with the freshest one at the named slot.
Thus, there is always only one named slot per event in the table. And the table occupies fixed memory size.

Here is the table of all possible events of the current firmware:

<br />

Event name      |   Value type                |   Description
--------------- | ----------------------------|------------------------------------------------------------------------------------- 
Button          |  boolean(true, false)       |   Notifies that user has pressed(true) or released(false) the button             |
ButtonStateCnt  |  integer(32bit)             |   A counter of button state changes (odd value means the button is pressed, even means it is released)
Menu            |  integer(32bit)             |   Notifies that menu has been selected (0 - none, 1 -gain menu , 2 - bridge menu, 3 - zero-calibration menu)
Zero            |  boolean(true, false)       |   Notifies that zero-calibration routine has been started(true) of finished(false)
Record          |  integer(32bit), RGBcolor   |   Notifies that a new record stamp has been created (random RGB color value)

<br />

The current event table can be obtained via default communication protocol in a JSON format by "je>" command:

Request/Response               |  Command
------------------------------ | -------------------------------------------------------------------------------------------------------------------------
request message:               |  je>\n
successive response message:   |  { "Button" : true, "ButtonStateCnt" : 3 }\n - indicates the board's button was pressed and shows its state counter: odd value means the button is pressed, even means it is released.

Please, see CommunicationProtocol.md for default communication protocol details.


