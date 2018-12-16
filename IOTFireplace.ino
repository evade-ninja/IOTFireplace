extern "C" { //used to allow setting hostname
#include "user_interface.h"
}

#include <ESP8266WiFi.h>
#include <MQTT.h>

#include "../../../wifipw.h" // Inside this file are defined ssid, pass for WiFi
#include "../../../mqtt-fireplace.h" // Inside this file are defined TOPIC, MQTT_SERVER, MQTT_HOSTNAME

#define INTERVAL 30000 // interval between MQTT updates
#define DEBOUNCE_COUNT 50
#define BLOCK_TIME 5000

#define RELAY_PIN D1
#define SWITCH_PIN D3

#define SERIAL_DEBUG

unsigned long lastUpdateTime = 0;
unsigned long pushAmt = 0;
unsigned long blockedTime = 0;

WiFiClient net;
MQTTClient client;

boolean fireplaceStatus = false; // false = off, true = on


void setup(){

    #ifdef SERIAL_DEBUG
    Serial.begin(112500);
    #endif

    pinMode(RELAY_PIN, OUTPUT);
    pinMode(SWITCH_PIN, INPUT_PULLUP);

    wifi_station_set_hostname("fireplace");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

   #ifdef SERIAL_DEBUG
    Serial.println("WiFi Started");
    #endif

    while(WiFi.status() != WL_CONNECTED){
        delay(500);
    }

    #ifdef SERIAL_DEBUG
    Serial.println("WiFi Connected");
    #endif

    client.begin(MQTT_SERVER, net);
    client.onMessage(messageReceived);

    delay(500);
    connect();

    #ifdef SERIAL_DEBUG
    Serial.println("MQTT Connected");
    #endif

    sendStatus();
}

void connect(){
    if(WiFi.status() != WL_CONNECTED){
        delay(1000);
    }

    while(!client.connect(MQTT_HOSTNAME)){
        #ifdef SERIAL_DEBUG
        Serial.print(".");
        #endif
        delay(1000);
    }
    client.subscribe(TOPIC_CMD);
}

void messageReceived(String &topic, String &payload){
    #ifdef SERIAL_DEBUG
    Serial.print("MQTT Topic: ");
    Serial.println(topic);
    Serial.print("MQTT Payload: ");
    Serial.println(payload);
    #endif

    if(topic == TOPIC_CMD && payload == "ON"){
        fireplaceOn();
    }
    if(topic == TOPIC_CMD && payload == "OFF"){
        fireplaceOff();
    }
    if(topic == TOPIC_CMD && payload == "STATUS"){
        sendStatus();
    }
}

void fireplaceOn(){
    #ifdef SERIAL_DEBUG
    Serial.println("Fireplace turned on.");
    #endif
    fireplaceStatus = true;
    digitalWrite(RELAY_PIN, HIGH);
    client.publish(TOPIC_STATUS, "ON");
}

void fireplaceOff(){
    #ifdef SERIAL_DEBUG
    Serial.println("Fireplace turned off");
    #endif
    fireplaceStatus = false;
    digitalWrite(RELAY_PIN, LOW);
    client.publish(TOPIC_STATUS, "OFF");
}

void loop(){
    client.loop();
    if(lastUpdateTime + INTERVAL < millis()){
        if(!client.connected()){
            connect();
        }
        sendStatus();
        lastUpdateTime = millis();
    }

    //Check the status of the manual input

    if(digitalRead(SWITCH_PIN) == LOW && blockedTime + BLOCK_TIME < millis()){
        pushAmt++;
        if(pushAmt >= DEBOUNCE_COUNT){
            //Fireplace should be toggled
            if(fireplaceStatus){
                fireplaceOff();
            }else{
                fireplaceOn();
            }
            blockedTime = millis();
        }
    }else{
        pushAmt=0;
    }
}

void sendStatus(){
    if(WiFi.status() == WL_CONNECTED){
        #ifdef SERIAL_DEBUG
        Serial.println("Publishing status");
        #endif

        String s = "";

        if(fireplaceStatus){
            s = "ON";
        }else{
            s = "OFF";
        }

        client.publish(TOPIC_STATUS, s);
    }
}