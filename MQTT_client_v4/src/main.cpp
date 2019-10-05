//***************************************************//
//****** Create a MQTT client using my library ******//
//***************************************************//

#include <Arduino.h>
#include <WiFi.h>
#if defined (__cplusplus)
extern "C"{
#endif
  #include <MQTTClient.h>
#if defined (__cplusplus)
}
#endif
#define LED_PIN 25

// AP settings
const char AP_ssid[] = "J_family";
const char AP_password[] = "1@&R_h&w_jomaa";
// MQTT Broker config
const char Broker_server[] = "192.168.1.103";
const int Broker_port = 1883;
const char Broker_user[] ="";
const char Broker_password[] ="";
// MQTT client settings
#if defined (__cplusplus)
extern "C"{
#endif
  mqttClient myMQTTClient;
#if defined (__cplusplus)
}
#endif
const char MQTT_client_id[] ="esp32Client";


void setup() {
  //************ config the serial communication ************//
  Serial.begin(115200);
  Serial.println("Start program");

  //************ config GPIO ************//
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  //************ config Wifi ************//
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(AP_ssid, AP_password);
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Estableshing connection...");
    delay(1000);
  }
  Serial.println("Device connected to AP");
  Serial.println(WiFi.localIP());
  //************ config MQTT client ************//
  // init MQTT client
  Serial.println("Init MQTT client");
  if(mqtt_client_init(&myMQTTClient, (char*)Broker_server, 1883, (char*)MQTT_client_id) != 0){
    Serial.println("Problem in init mqtt client");
  }
  // connect to Broker
  /*if(mqtt_client_connect(&myMQTTClient) != 0){
    Serial.println("Problem in sending connection request");
  }*/
  Serial.println("Set username and password for the MQTT client");
  if(mqtt_client_set_usernameAndPassword(&myMQTTClient, "my username esp32", "my password esp32") != 0){
    Serial.println("Problem in setting up a username and password");
  }
  Serial.println("Set the last testament for the MQTT client");
  if(mqtt_client_set_lastTestament(&myMQTTClient, "will topic esp32", "will message esp32", false, 0) != 0){
    Serial.println("Problem in sseting up the last testament");
  }
  Serial.println("Send connection request");
  if(mqtt_client_connect_adavance(&myMQTTClient, false, (uint16_t)120) != 0){
    Serial.println("Problem in sending connection request");
  }
  Serial.println("Publish message");
  if(mqtt_client_publish(&myMQTTClient, (char*)"testTopic", (char*)"message for the topic", 0) != 0){
    Serial.println("Problem in publishing a message");
  }
  Serial.println("Stopping connection...");
}

void loop() {
  delay(1000);
}