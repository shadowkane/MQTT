#include "MQTTClient.h"

// init the client struct and set its elements to the default values
// create a socket and connect to server (sokcet connection and not MQTT connection)
int mqtt_client_init(mqttClient *client, char* brokerURL, int portNumber, char* clientID){
    // init the broker configuration of the client
    client->brokerAddr = malloc(strlen(brokerURL)*sizeof(char));
    strcpy(client->brokerAddr,brokerURL);
    client->brokerPort = portNumber;
    client->clientID = malloc(strlen(clientID)*sizeof(char));
    client->clientID = clientID;
    // set the client struct to the default states
    client->userName = "";
    client->password = "";
    client->willTopic ="";
    client->willMessage ="";
    client->newSession = true;
    client->willRetainMessage = false;
    client->willQos = 0;
    client->keepAlive = defaultKeepAlive;
    // set the private setup of the broker
    memset(&client->__brokerAddr, 0, sizeof(client->__brokerAddr));
    client->__brokerAddr.sin_family = AF_INET;
    //lwip_inet_pton(AF_INET, client->brokerAddr, &client->__brokerAddr);
    client->__brokerAddr.sin_port = lwip_htons(client->brokerPort);
    client->__brokerAddr.sin_addr.s_addr = inet_addr(client->brokerAddr);
    client->__brokerAddr.sin_len = sizeof(client->__brokerAddr);
    // open socket for this client and save its file descriptor
    client->__client_socket_file_descriptor = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if(client->__client_socket_file_descriptor < 0){
        perror("Error openning socket: ");
        return -1;
    }
    if(lwip_connect(client->__client_socket_file_descriptor, (struct sockaddr*) &(client->__brokerAddr), sizeof(client->__brokerAddr)) < 0){
        perror("Error in socket connection: ");
        return -1;
    }
    return 0;
}

// set the username and the password that should be used to connect to the broker before sending the connection request
int mqtt_client_set_usernameAndPassword(mqttClient *client, char* userName, char* password){
    if(strcmp(userName,"")!=0 && userName != NULL){
        client->userName = malloc(strlen(userName)*sizeof(char));
        client->userName = userName;
        if(strcmp(password,"")!=0 && password != NULL){
            client->password = malloc(strlen(password)*sizeof(char));
            client->password = password;
        }
        return 0;
    }else{
        perror("username can't be empty");
        return -1;
    }
    
    return 0;
}

// set the will message (last testamenet) that the client should send to the broker with it's flags (retain and QoS flag) (message payload and message topic both must be exist)
int mqtt_client_set_lastTestament(mqttClient *client, char* topic, char* message, bool retain, short qos){
    if(strcmp(topic,"")!=0 && topic != NULL){
        if(strcmp(message,"")!=0 && message != NULL){
            client->willTopic = malloc(strlen(topic)*sizeof(char));
            strcpy(client->willTopic, topic);
            client->willMessage = malloc(strlen(message)*sizeof(char));
            strcpy(client->willMessage, message);
            client->willRetainMessage = retain;
            if(qos < 0 || qos > 2){
                perror("QoS must be between 0 and 2, default value is used (0).");
                client->willQos = 0;
            }else{
                client->willQos = qos;
            }
            return 0;   
        }
        perror("Will message must not be empty!");
        return -1;
    } 
    perror("Will topic must not be empty!");
    return -1;
}

int mqtt_client_connect_adavance(mqttClient *client, bool newSession, uint16_t keepAlive){
    //***** Fixed header *****//
    // fixed header has 2 bytes, one for the MQTT control packet type (4 bits)+ reserved for flag (4 bits). second for remaining length
    int fixed_headerLen = 2;
    uint8_t fixed_header[2];
    fixed_header[0] = connectHeader | qos0Flag;
    // fixed_header[1] is the remaining length and to get the right length we need to create first the variable header and the payload.

    //***** Variable header *****//
    int variable_headerLen = 10;
    uint8_t variable_header[10];
    // protocol name length (2bytes)
    variable_header[0] = 0x00;
    variable_header[1] = 0x04;
    // protocol name
    variable_header[2] = 'M';
    variable_header[3] = 'Q';
    variable_header[4] = 'T';
    variable_header[5] = 'T';
    // protocol version
    variable_header[6] = 4;
    // connection flag
    variable_header[7] = 0x00;
    //  username
    if(strcmp(client->userName,"")!=0 && client->userName != NULL){
        variable_header[7] |= usernameFlag;
        //  password (available only if the username is used)
        if(strcmp(client->password,"")!=0 && client->password != NULL){
            variable_header[7] |= passwordFlag;
        }
    }
    //  testament message's presistance
    if (client->willRetainMessage){
        variable_header[7] |= willRetainFlag;
    }
    //  testament message's QoS
    if (client->willQos == 0){
        variable_header[7] |= willQos0Flag;
    }else if (client->willQos == 1){
        variable_header[7] |= willQos1Flag;
    }else if (client->willQos == 2){
        variable_header[7] |= willQos2Flag;
    }
    //  testament message availablity
    if(strcmp(client->willMessage,"")!=0 && client->willMessage != NULL){
        variable_header[7] |= willFlag;
    }
    // clean session
    if (!client->newSession){
        variable_header[7] |= cleanSessionFlag;
    }
    // keep alive in 2 bytes
    variable_header[8] = keepAlive >> 8; // MSB
    variable_header[9] = keepAlive & 0x00ff; // LSB (60 seconds) 

    //***** payload *****//
    // I just sacrificed 2 character from the max length of the payload to use the unsigned int of 16 bits type for the payloadLen var. I guess it won't make a big difference.
    //  client id
    uint16_t payloadLen = 2 + strlen(client->clientID);
    
    // max size of the payload is the max value the remaining header in the fixed header can reach which is 0xFFFF (2 bytes);
    //uint8_t* payload[0xFFFF];
    // because the max length is too big to be reserved in memory, we will use a dynamic allocation
    uint8_t* payload = malloc(sizeof(uint8_t)* payloadLen);
    
    int currentPos = 0;
    
    // set the client id length (2bytes)
    payload[currentPos] = ((uint16_t)strlen(client->clientID)) >> 8; // MSB
    currentPos++;
    payload[currentPos] = ((uint16_t)strlen(client->clientID)) & 0x00ff; // LSB
    currentPos++;
    // set the client id
    int i = 0;
    for(; currentPos<payloadLen; currentPos++){
        payload[currentPos] = client->clientID[i];
        i++;
    }
    // set the will topic and message if availabe (the will flag was set to 1)
    if(variable_header[7] & willFlag){
        // will topic
        payloadLen += 2 + strlen(client->willTopic);
        payload = realloc(payload, sizeof(uint8_t) * payloadLen);
        // set the will topic length (2bytes)
        payload[currentPos] = ((uint16_t)strlen(client->willTopic)) >> 8; // MSB
        currentPos++;
        payload[currentPos] = ((uint16_t)strlen(client->willTopic)) & 0x00ff; // LSB
        currentPos++;
        // set the will topic payload
        i = 0;
        for(;currentPos<payloadLen; currentPos++){
            payload[currentPos] = client->willTopic[i];
            i++;
        }
        // will message
        payloadLen += 2 + strlen(client->willMessage);
        payload = realloc(payload, sizeof(uint8_t) * payloadLen);
        // set the will message length (2bytes)
        payload[currentPos] = ((uint16_t)strlen(client->willMessage)) >> 8; // MSB
        currentPos++;
        payload[currentPos] = ((uint16_t)strlen(client->willMessage)) & 0x00ff; // LSB
        currentPos++;
        i = 0;
        for(;currentPos<payloadLen; currentPos++){
            payload[currentPos] = client->willMessage[i];
            i++;
        }
    }
    // set the username if availabe (the username flag was set to 1)
    if(variable_header[7] & usernameFlag){
        payloadLen += 2 + strlen(client->userName);
        payload = realloc(payload, sizeof(uint8_t) * payloadLen);
        // set the will topic length (2bytes)
        payload[currentPos] = ((uint16_t)strlen(client->userName)) >> 8; // MSB
        currentPos++;
        payload[currentPos] = ((uint16_t)strlen(client->userName)) & 0x00ff; // LSB
        currentPos++;
        // set the will topic payload
        i = 0;
        for(;currentPos<payloadLen; currentPos++){
            payload[currentPos] = client->userName[i];
            i++;
        }
    }
    // set the password if availabe (the password flag was set to 1)
    if(variable_header[7] & passwordFlag){
        payloadLen += 2 + strlen(client->password);
        payload = realloc(payload, sizeof(uint8_t) * payloadLen);
        // set the will topic length (2bytes)
        payload[currentPos] = ((uint16_t)strlen(client->password)) >> 8; // MSB
        currentPos++;
        payload[currentPos] = ((uint16_t)strlen(client->password)) & 0x00ff; // LSB
        currentPos++;
        // set the will topic payload
        i = 0;
        for(;currentPos<payloadLen; currentPos++){
            payload[currentPos] = client->password[i];
            i++;
        }
    }

    // Now after we know the right length of the variable header and the payload we can set the remaining header byte.
    fixed_header[1] = variable_headerLen + payloadLen;

    // Create the whole packet to send to broker
    int packetLen = fixed_headerLen + variable_headerLen + payloadLen;
    uint8_t* packet = malloc(sizeof(uint8_t)*packetLen);
    for(int i=0; i<packetLen; i++){
        if(i<fixed_headerLen){
        packet[i] = fixed_header[i];
        }else if(i<(fixed_headerLen+variable_headerLen)){
        packet[i] = variable_header[i-fixed_headerLen];
        }else{
        packet[i] = payload[i-(fixed_headerLen+variable_headerLen)];
        }
    }
    if(lwip_write(client->__client_socket_file_descriptor, packet, sizeof(uint8_t)*packetLen) < 0){
        perror("Sending connetion request failed: ");
        return -1;
    }

    // wait for response from the broker. In future change the recv() function whith another one that doesn't block.
    // Waiting for a response from now and before going any further because it's pointless to wait for a response from a broker in the loop when the client didn't even get a confirmation about the connection.
    client->__lastActiveTime = millis();
    uint8_t receivedData[128];
    int receivedDataLen = 0;
    while((receivedDataLen = lwip_recv(client->__client_socket_file_descriptor, receivedData, sizeof(receivedData), 0)) == 0){
        uint32_t currentTime = millis();
        if((currentTime - client->__lastActiveTime) > MQTT_SOCKET_TIMEOUT * 1000UL){
            perror("MQTT timeout waiting for respose from broker.");
            client->__state = MQTT_CONNECTION_TIMEOUT_ERROR;
            lwip_close(client->__client_socket_file_descriptor);
            return MQTT_CONNECTION_TIMEOUT_ERROR;
        }
    }
    // the connectACk has 4 byte: 2 bytes for the fixed header (byte 1 for flags and message header and byte 2 for the remaining length) and 2 bytes for variable header (byte 1 for the session presend and byte 2 for the returned code)
    if(receivedDataLen == 4){
        if(receivedData[0] == connectAckHeader){
            if(receivedData[3] == connectionAccepted){
                client->__state = MQTT_CONNECTED;
                return MQTT_CONNECTED;
            }
            else if(receivedData[3] == badProtocol){
                client->__state = MQTT_CONNECT_BAD_PROTOCOL;
                lwip_close(client->__client_socket_file_descriptor);
                return MQTT_CONNECT_BAD_PROTOCOL;
            }
            else if(receivedData[3] == badClientId){
                client->__state = MQTT_CONNECT_BAD_CLIENT_ID;
                lwip_close(client->__client_socket_file_descriptor);
                return MQTT_CONNECT_BAD_CLIENT_ID;
            }
            else if(receivedData[3] == brokerUnavailable){
                client->__state = MQTT_CONNECT_UNAVAILABLE;
                lwip_close(client->__client_socket_file_descriptor);
                return MQTT_CONNECT_UNAVAILABLE;
            }
            else if(receivedData[3] == badCredentials){
                client->__state = MQTT_CONNECT_BAD_CREDENTIALS;
                lwip_close(client->__client_socket_file_descriptor);
                return MQTT_CONNECT_BAD_CREDENTIALS;
            }
            else if(receivedData[3] == unauthorizedUser){
                client->__state = MQTT_CONNECT_UNAUTHORIZED;
                lwip_close(client->__client_socket_file_descriptor);
                return MQTT_CONNECT_UNAUTHORIZED;      
            }     
           else{
                lwip_close(client->__client_socket_file_descriptor);
                return -1;
            }
        }
    }else{
        lwip_close(client->__client_socket_file_descriptor);
        return MQTT_CONNECTION_FAILED_ERROR;
    }
}

// connect to broker with default setting (which mean the default value of keep alive periode and with a new session (clean session) )
int mqtt_client_connect(mqttClient *client){
    return mqtt_client_connect_adavance(client, true, defaultKeepAlive);
}

int mqtt_client_publish(mqttClient *client, char *topic, char *message, int Qos){
    if(client->__state == MQTT_CONNECTED){
    //******** Fixed header ********//
        int fixed_headerLen = 2;
        uint8_t fixed_header[2];
        fixed_header[0] = publishHeader;
        
        switch (Qos){
        case 0:
            fixed_header[0] |= qos0Flag;
            break;
        
        default:
            perror("Unknow QoS are used to publish a message");
            return -1;
            break;
        }

        //******** Variable header ********//
        int variable_headerLen = strlen(topic) + 2 ; // 2 topic name length in 2 byte + length of the topic name. (note: if QoS > 0 then the variable header will include another information which is the message id in 2 byte at the end of the variable header)
        uint8_t* variable_header = malloc(sizeof(uint8_t)*variable_headerLen);
        // Topic name length (2bytes)
        variable_header[0] = (variable_headerLen-2) >> 8;
        variable_header[1] = (variable_headerLen-2) & 0x00ff;
        // Topic name
        for(int i=0; i<variable_headerLen-2; i++){
            variable_header[i+2] = topic[i];
        }
        // Message id
        variable_header[variable_headerLen-1] = availableMessageId >> 8;
        variable_header[variable_headerLen] = availableMessageId & 0x00ff;

        //******** payload ********//
        // I just sacrificed 2 character from the max length of the client id to use the unsigned int of 16 bits type for the payloadLen var. I guess it won't make a big difference.
        uint16_t payloadLen = strlen(message);
        uint8_t* payload = malloc(sizeof(uint8_t)*(payloadLen));
        memcpy(payload, message, payloadLen * sizeof(uint8_t));
        // Remaining length 
        fixed_header[1] = variable_headerLen + payloadLen;

        //******** create the whole packet to send to broker ********//
        int packetLen = fixed_headerLen + variable_headerLen + payloadLen;
        uint8_t* packet = malloc(sizeof(uint8_t)*packetLen);
        for(int i=0; i<packetLen; i++){
            if(i<fixed_headerLen){
            packet[i] = fixed_header[i];
            }else if(i<(fixed_headerLen+variable_headerLen)){
            packet[i] = variable_header[i-fixed_headerLen];
            }else{
            packet[i] = payload[i-(fixed_headerLen+variable_headerLen)];
            }
        }
        if(lwip_write(client->__client_socket_file_descriptor, packet, sizeof(uint8_t)*packetLen) < 0){
            perror("Sending connetion request failed: ");
            return -1;
        }
        return 0;
    }
    return -1;
}