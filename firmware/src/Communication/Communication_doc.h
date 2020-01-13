/*!
 * \file
 * \brief this file is used only to generate the documentation
 *  
 * \page Communication_page Communication
 *
 * The communication with the master device is implemented by a master-slave model.
 * Master device sends a command request and waiting for response from the board.
 * Handling of corresponding communication protocols and commands dispatching is implemented via Port (basic CStdPort),
 * Stream (basic CFrmStream) and Command Dispatcher ( CCmdDispatcher ) concepts.
 *
 * The Port is responsible for handling specific communication protocol. Being connected to a serial device object CSerial
 * as event listener it parses the incoming message to match the communication protocol. When correct message is detected it
 * forms a protocol-independent request in CCmdCallDescr and send it to CCmdDispatcher object.
 * Thus several communication protocols can be handled at once by creating a number of specific Port objects
 *
 * The Stream is used for storing call parameters and data in a serialized form and provide a mechanism for retrieving/storing
 * primitive data types (int, float, std::string, etc) from/to the stream.
 * The current stream object type determines a storage format (text, binary, etc)
 *
 * The Command Dispatcher is used to dispatch command request in protocol-independent format to corresponding command handler.
 *
 * Finally, to implement primary communication protocol based on "access point" concept (see CommunicationProtocol.md)
 * its necessary to transform a command request from protocol-independent format to C/C++ function/class method call with
 * "get" and "set" signatures like "APtype get()" and "void set(APtype)" where APtype is an access point type.
 * For this purpose two special command handlers are used: CCmdSGHandler - for binding to class methods,
 * CCmdSGHandlerF - for binding to functions or static class methods.
 * Thus a communication "access point" can be created from already existing class methods with appropriate signature by binding them with
 * CCmdSGHandler for example
 *
 *
 * Since normally SPI bus is used for communication a kind of flow-control is also required. It is realized via CSyncSerComFSM
 */
 
 
