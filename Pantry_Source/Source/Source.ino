#include "heltec.h"
#include "Vector.h"
// Load Wi-Fi library
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define BAND    868E6  //change this Band and set Class C
#define NUM_NODES 4
#define PANTRY "5"
#define MESSAGE_LENGTH 24
#define DELAY 500
#define TIMEOUT 30*1000
#define PUSHED 5

const byte interruptPin = 0;
String userInput = "";
String id="03";


Vector<Vector<String>> orders;
Vector<String> orderContainer[NUM_NODES];

// Replace with your network credentials
String _ssid = "ESP32-Access-Point"+id;
const char* ssid     = _ssid.c_str();
const char* password = "123456789";

// Set web server port number to 80
WebServer server(80);

bool pushedButton = false;
Vector<String> pushedMessages;
String pushedMessagesContainer[NUM_NODES];

unsigned long start;

void handleOrder(){
  userInput = server.arg("plain");        
  server.send(200, "text/plain", "ORDER"+userInput);
  Serial.println("message: "+userInput);
}

void setup() {
  orders.setStorage(orderContainer, NUM_NODES, NUM_NODES);
  pushedMessages.setStorage(pushedMessagesContainer,NUM_NODES,NUM_NODES);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, FALLING);
  //WIFI Kit series V1 not support Vext control
  Heltec.begin(true , true , true , true , BAND );
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0,0,id);
  Heltec.display->display();
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/order",HTTP_POST, handleOrder);

  // server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started "+id);
  
  Serial.println("id: "+id);

  start = millis();

}
void blink() {
  pushedButton = true;
}

bool recv = true;

String parseLength(int length){
  String parsedLength = String(length);
  while(parsedLength.length() < 4){
    parsedLength = "0" + parsedLength;
  }
  return parsedLength;
}

//While sending a msg only accept from those which have id less than yours.

void loop() {
  server.handleClient();

  int packetSize = LoRa.parsePacket();
  String incoming = "";
  if (packetSize) {
    Serial.print("Received packet: ");
    while (LoRa.available()) {
      incoming+=(char)LoRa.read();
    }
    int srcNodeIndex = incoming.substring(2,4).toInt() - 1;
    Serial.println("recieved from node index: " + String(srcNodeIndex));
    Serial.println(id+" received "+incoming);
    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());

    String prevId = incoming.substring(0,2);
    Serial.println("prev id: "+ prevId + " " + prevId.toInt());
    if(prevId.toInt() < id.toInt()){
      if(incoming.substring(8) == "First"){
        if(orders[srcNodeIndex].size() == 0) {
          int count = incoming.substring(4,8).toInt();
          String* orderChunks = new String[count];
          orders[srcNodeIndex].setStorage(orderChunks, count+1, count+1);
          Serial.println("size of orders in srcNode: " + orders[srcNodeIndex].size());
          orders[srcNodeIndex][0] = incoming;
        }
      }
      else if(incoming.substring(8) == "Push"){
        pushedMessages[srcNodeIndex] = incoming; 
      }
      else {
        int sequenceNo = incoming.substring(4,8).toInt();
        if(orders[srcNodeIndex][sequenceNo+1].length() == 0){
          orders[srcNodeIndex][sequenceNo+1] = incoming;
        }
      }
    }
    start = millis();
  }
  if(userInput!=""){
    int length = userInput.length();
    int len = ceil((double)length/MESSAGE_LENGTH);
    String firstMessage = id;
    firstMessage += id;
    firstMessage += parseLength(len);
    firstMessage += "First";

    LoRa.beginPacket();
    LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
    LoRa.print(firstMessage);
    LoRa.endPacket();    
    delay(DELAY);

    for(int x = 0;x < len;x++){
      String temp = id + id; // prev and src are same
      temp += parseLength(x);
      temp += userInput.substring((x*MESSAGE_LENGTH),min((x+1)*MESSAGE_LENGTH,length));
      LoRa.beginPacket();
      LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
      LoRa.print(temp);
      LoRa.endPacket();    
      delay(DELAY);
    }
    userInput = "";
  }
  if(pushedButton){
    String firstMessage = id;
    firstMessage += id;
    firstMessage += parseLength(0);
    firstMessage += "Push";

    LoRa.beginPacket();
    LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
    LoRa.print(firstMessage);
    LoRa.endPacket();    
    delay(DELAY);

    pushedButton = false;
  }

  if(millis()-start>TIMEOUT){
    for(int x = 0;x<orders.size();x++){
      for(auto message: orders[x]){
        if(message!=""){
          String prevId = message.substring(0,2);
          Serial.println("prev id: "+ prevId + " " + prevId.toInt());
          if(prevId.toInt() < id.toInt()){
            for(int j = 0; j < id.length(); j++){
              message[j] = id[j];
            }
            LoRa.beginPacket();
            LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
            LoRa.print(message);
            LoRa.endPacket();
            Serial.println(id+" message sent with: " +message);
            delay(DELAY);
          }
        }
      }
      orders[x].clear();
    }
    for(int x = 0;x<pushedMessages.size();x++){
      String message = pushedMessages[x];
      if(message!=""){
        String prevId = message.substring(0,2);
        Serial.println("prev id: "+ prevId + " " + prevId.toInt());
        if(prevId.toInt() < id.toInt()){
          for(int j = 0; j < id.length(); j++){
            message[j] = id[j];
          }
          LoRa.beginPacket();
          LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
          LoRa.print(message);
          LoRa.endPacket();
          Serial.println(id+" message sent with: " +message);
          delay(DELAY);
        } 
        pushedMessages[x] = "";
      }
    }
    start = millis();
  }
      
}