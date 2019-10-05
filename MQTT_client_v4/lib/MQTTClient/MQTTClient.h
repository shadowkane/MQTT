#include <stdint.h>
#include <string.h>

#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <errno.h>

#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

//**************************************************************************** Symbolic constante ****************************************************************************//

// Maximum time the client should wait for a response from the broker. it's different from the keep alive field which mean the maximum time the client can wait between commends or he should send a ping to keep the connectom alive between him and the broker
#ifndef MQTT_SOCKET_TIMEOUT
#define MQTT_SOCKET_TIMEOUT 15
#endif

//***** Client state *****//
// Client didn't get a response from broker for a predefined (preset) periode which is defined in "MQTT_SOCKET_TIMEOUT"
#ifndef MQTT_CONNECTION_TIMEOUT_ERROR
#define MQTT_CONNECTION_TIMEOUT_ERROR -4
#endif
// Client lost connection with the broker
#ifndef MQTT_CONNECTION_LOST_ERROR
#define MQTT_CONNECTION_LOST_ERROR -3
#endif
// Start connection between client and broker failed
#ifndef MQTT_CONNECTION_FAILED_ERROR
#define MQTT_CONNECTION_FAILED_ERROR -2
#endif
// Client disconnected from the broker
#ifndef MQTT_DISCONNECTED
#define MQTT_DISCONNECTED -1
#endif
// Client connected to the broker
#ifndef MQTT_CONNECTED
#define MQTT_CONNECTED 0
#endif

#ifndef MQTT_CONNECT_BAD_PROTOCOL
#define MQTT_CONNECT_BAD_PROTOCOL -5
#endif

#ifndef MQTT_CONNECT_BAD_CLIENT_ID
#define MQTT_CONNECT_BAD_CLIENT_ID -6
#endif

#ifndef MQTT_CONNECT_UNAVAILABLE
#define MQTT_CONNECT_UNAVAILABLE -7
#endif

#ifndef MQTT_CONNECT_BAD_CREDENTIALS
#define MQTT_CONNECT_BAD_CREDENTIALS -8
#endif

#ifndef MQTT_CONNECT_UNAUTHORIZED
#define MQTT_CONNECT_UNAUTHORIZED -9
#endif


typedef struct mqttClient mqttClient;

struct mqttClient{
    //****** private setup ******//
    int __client_socket_file_descriptor;
    struct sockaddr_in __brokerAddr;
    uint32_t __lastActiveTime; // the time in milliSeconds when the system send a request/response (commend) to the broker
    uint32_t __state; // the current state of the mqtt client
    //****** client configuration ******//
    char* clientID; // required (must be set by the user)
    bool newSession; // optional (default value is true which mean it's new session) {user can use this variable if he wants to request a new session or old session from the broker in the connection request. and a response (connection ack) from the broker will determine if this session is new or not}
    bool willRetainMessage; // optional (default is false);
    short willQos; // optional (default is 0);
    char* willTopic; // optional
    char* willMessage; // optional
    uint16_t keepAlive; // optional (default is 60 sec)
    
    //****** broker configuration ******//
    char* brokerAddr; // required (must be set by the user)
    int brokerPort; // required (must be set by the user)
    char* userName; // optional (Null mean this variable not in use)
    char* password; // optional (Null mean this variable not in use)
};

//**************************************************************************** Fixed header ****************************************************************************//
                                            //*************************** Fixed header struction ***************************//
// The fixed header uses 2 Bytes :
//      + Byte 1 divided in 2 digits:
//          - First 4 bits (0-3) for the flag header.
//          - Second 4 bits (4-7) for the message type header.
//      + Byte 2: remaining length.

                                            //*************************** Fixed header definition ***************************//
// Because we don't have a 4 bits type so we use the smallest type which is the unsigned int in 8 bits
// Flag header (First 4 bits of the first byte of the header)
// DUP header is 1 bit (bit 3) 
//      0 (0xxx): mean this is the client first attempt to send MQTT publish control message:
//      1 (1xxx): to indicate that this message is duplicated which mean the client is sending this message again because something wrong happen to the original one
// QoS header is 2 bits (bit 1-2)
//      x00x: QoS 0 (fire and forget)
//      x01x: QoS 1 (at least onece)
//      x10x: QoS 2 excatly onece)
//      x11x it's reserver for future implimentation maybe
// Retain is 1 bit (bit 0) 
//      0 (xxx0): broker will not store the message.
//      1 (xxx1): tell the broker to hold or store the message unless they are no client subscribe to that topic
// in this scope i will set x to 0 so to make a complite flag header just do the combination using the operation bitwise OR
static uint8_t dupFlag = 0b00001000;
static uint8_t qos0Flag = 0b00000000;
static uint8_t qos1Flag = 0b00000010;
static uint8_t qos2Flag = 0b00000100;
static uint8_t retainFlag = 0b00000001;

// message type header
static uint8_t reserverHeader = 0x00; // 0xF0
static uint8_t connectHeader = 0x10; // Client send a connection command to the broker
static uint8_t connectAckHeader = 0x20; // client receive a connection acknowledge from the broker
static uint8_t publishHeader = 0x30; // client publish a message to the broker
static uint8_t publishAckHeader = 0x40; // client receive a publish acknowledge from the broker (for the QoS 1)
static uint8_t publishRecHeader = 0x50;  // client receive publish receive packet from the broker (for the QoS 2)
static uint8_t publishRelHeader = 0x60; // client send publish release packet from the broker (for the QoS 2)
static uint8_t publishCompHeader = 0x70; // client receive a publish complete packet from the procker (for the QoS 2)
static uint8_t subscribeHeader = 0x80; // client send a subscribe request to a topic to the broker
static uint8_t subscribeAckHeader = 0x90; // client receive a subscribe acknowledge from the broker
static uint8_t unsubscribeHeader = 0xA0; // client send unsubscribe request from a topic to the broker
static uint8_t unsubscribeAckHeader = 0xB0; // client receive unsubscribe acknowledge from the broker
static uint8_t pingRequestHeader = 0xC0; // client send ping request to the broker
static uint8_t pingResponseHeader = 0xD0; // client receive a ping response from the broker
static uint8_t disconnectHeader = 0xE0; // client send a disconnection command to the broker


//**************************************************************************** Variable header ****************************************************************************//
                                            //*************************** variable header struction ***************************//
// Variable header is present in all kind of messages except the ping request, ping response and disconnect messages.
// Variable header struction change from message to another.


                                            //*************************** variable header definition ***************************//
        //******** Connection message ********//
// Protocol length (2 bytes): which is the length of the protocol name and in our case it's 4 bytes (length of "MQTT" = 4).
// Protocol name (n bytes): it's MQTT.
// Version (1 byte): the version of MQTT in use. i will use the version 4 which is the 3.1.1
// Connect flag (1 byte):
//      Username flog (bit 7): 
//          0: mean no username is present in the payload
//          1: mean a username must be present in the payload
//      Password flag (bit 6):
//          0: mean no password is present in the payload
//          1: mean a password must be present in the payload
//      Will retain flag (bit 5): if the will flag is 0 then will retain must be set to 0.
//          0: mean the broker will not retain the message
//          1: mean the broker will retain the message
//      Will QoS flag (bits 3-4): if the will flag is 0 then will Qos must be set to 00
//          00: QoS 0
//          01: QoS 1
//          10: QoS 2
//          11: reserved
//      Will flag (bit 2):
//          0: mean the broker will not use (not look) at the will retain or will Qos flags and will topic and will message must not be present in the payload 
//          1: mean the broker will use (take into consideration) the will retain and will Qos flags and will topic and will message must be present in the payload
//      Clean session flag (bit 1):
//          0: mean the broker must not clean the session of the client and carry last session states. If no session was exist then just start new one.
//          1: mean the broker must clean (discard) the previous session states and start a new one for the client.
//      bit 0 is reserver for future implementation
// Keep alive (2 bytes): The duration in seconds which the subscriber sends ping requests to broker to maintain connection to stay alive. The dafault keep alive duration is 60 sec.

static uint16_t protocolNameLength = 4;
static uint8_t protocolName[5] = "MQTT";
static uint8_t mqttVersion = 4;
static uint8_t usernameFlag = 0b10000000;
static uint8_t passwordFlag = 0b01000000;
static uint8_t willRetainFlag = 0b00100000;
static uint8_t willQos0Flag = 0b00000000;
static uint8_t willQos1Flag = 0b00001000;
static uint8_t willQos2Flag = 0b00010000;
static uint8_t willFlag = 0b00000100;
static uint8_t cleanSessionFlag = 0b00000010;
static uint16_t defaultKeepAlive = 60;

        //******** Connection acknowledge message ********//
// acknowledgement flag (1 byte):
//      * Reserved (7-1 bits): not in use, maybe for future implementation
//      * Session present occupe 1 bit (bit 0) set by the broker which indicate the state of client's session and it has 3 different meaning depends on the clear session flag sent by the client in the connection request:
//              + if the client sent a connection request with clean session flag = 1:
//                      - Session present = 0: it's the only respose the server will sent which mean the broker accpet the connection request and client can connect to it.
//              +if the client sent a connection request with clean session flag = 0:
//                      - Session present = 0: mean the broker accpet connection and it's already has stored states for this client.
//                      - Session present = 1: mean the broker accpet connection but it didn't found any stored states for this client.
// Return code(1 byte): which is the response from the broker to the this client
//      * 0x00 (0): Connect accepted
//      * 0x01 (1): Bad protocol, which mean the broker can't support the request sent from the client
//      * 0x02 (2): Bad client id, the client id itself is correct but it was rejected by the broker
//      * 0x03 (3): Server unavailable which mean the MQTT service is not available
//      * 0x04 (4): Bad credentials which mean wrong username and password used for authentication
//      * 0x05 (5): Unauthorized user which mean client is not allowed (authorized) to connect with the broker
//      * 0x06-0xFF (6-255) : nothing (maybe for future impelementation)

// if the client sent request with clear session flag = 1, use this constant to do the test
static const uint8_t sessionPresent = 0; 
// if the client sent request with clear session flag = 0, use this constant to do the test
static const uint8_t sessionPresentWithStoredSate = 0;
static const uint8_t sessionPresentWithoutStoredState = 1;

static const uint8_t connectionAccepted = 0x00;
static const uint8_t badProtocol = 0x01; 
static const uint8_t badClientId = 0x02; 
static const uint8_t brokerUnavailable = 0x03; 
static const uint8_t badCredentials = 0x04; 
static const uint8_t unauthorizedUser = 0x05; 

        //******** Publish message ********//
static uint16_t availableMessageId = 0;

//**************************************************************************** Payload ****************************************************************************//
                                            //*************************** Payload struction ***************************//
// Because the payload depends on the client itself and has no static information for all clients so it will be defined in the mqttClient struct.
// Payload is present only in connect, publish, subscribe, subscribe acknowledge and unsubscribe messages.
// Payload struction change from message to another, the description below demonstrate the payload struction for each message type:
        //******** Connection message ********//
//  * Client Identifier Length (require).
//  * Client Identifier (require).
//  * Will Topic (optional).
//  * Will Message optional
//  * User Name (optional).
//  * Password (optional).

        //******** Publish message ********//
//  * Message (n bytes).

        //******** Subscribe message ********//
//  * Topic Length (2 bytes).
//  * Topic (n bytes).
//  * Requested QOS (2 bits).

        //******** Subscribe acknowledge message ********//
//  * The granted (given) QoS from the broker (2 bits).

        //******** Unsubscribe message ********//
//  * Topic (n bytes).

//********************* functions *********************//
int mqtt_client_init(mqttClient *client, char* brokerURL, int portNumber, char* clientID);
int mqtt_client_set_usernameAndPassword(mqttClient *client, char* userName, char* password);
int mqtt_client_set_lastTestament(mqttClient *client, char* topic, char* message, bool retain, short qos);
int mqtt_client_connect(mqttClient *client);
int mqtt_client_connect_adavance(mqttClient *client, bool newSession, uint16_t keepAlive);
int mqtt_client_publish(mqttClient *client, char *topic, char *message, int Qos);

#endif