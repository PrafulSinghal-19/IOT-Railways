#include "heltec.h"

// Load Wi-Fi library
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define BAND    868E6  //change this Band and set Class C

#define PANTRY "5"
#define MESSAGE_LENGTH 24

const byte interruptPin = 0;
String userInput = "";
String id="02";


// Replace with your network credentials
String _ssid = "ESP32-Access-Point"+id;
const char* ssid     = _ssid.c_str();
const char* password = "123456789";

// Set web server port number to 80
WebServer server(80);


void handleOrder(){
  userInput = server.arg("plain");        
  server.send(200, "text/plain", "ORDER"+userInput);
  Serial.println("message: "+userInput);
}

void setup() {
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, FALLING);
  //WIFI Kit series V1 not support Vext control
  Heltec.begin(true , true , true , true , BAND );

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

}
void getInput(){
  userInput = "id";
}
void blink() {
  getInput();
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
    Serial.println(id+" received "+incoming);
    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());
    if(incoming.substring(8) == "First"){
      int count = incoming.substring(4,8).toInt();
      String messageArray[count+1];
      messageArray[0] = incoming;
      int packetCount = 0;
      while(packetCount < count){
        while(!LoRa.parsePacket());
        String temp = "";
        while (LoRa.available()) {
          temp+=(char)LoRa.read();
        }
        if(temp.substring(2,4) != incoming.substring(2,4)) continue; // skips the packet when not from the source which send the first packet
        // replace prev-id with the current id
        Serial.println(id+"temp received with " + String(packetCount) + " : " +temp);
        Serial.print("RSSI: ");
        Serial.println(LoRa.packetRssi());
        messageArray[packetCount+1] = temp;
        packetCount = packetCount + 1;
        if(temp.substring(4,8).toInt() == count -1) break; // check for last packet if some packet missed
      }
      String prevId = incoming.substring(0,2);
      Serial.println("prev id: "+ prevId + " " + prevId.toInt());
      if(prevId.toInt() < id.toInt()){
        for(int i = 0; i < count+1; i++){
          for(int j = 0; j < id.length(); j++){
            messageArray[i][j] = id[j];
          }
          LoRa.beginPacket();
          LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
          LoRa.print(messageArray[i]);
          LoRa.endPacket();
          Serial.println(id+"message sent with index:" + String(i) + " : " +messageArray[i]);
          delay(1000);
        }
      }
    }
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
    delay(1000);

    for(int x = 0;x < len;x++){
      String temp = id + id; // prev and src are same
      temp += parseLength(x);
      temp += userInput.substring((x*MESSAGE_LENGTH),min((x+1)*MESSAGE_LENGTH,length));
      LoRa.beginPacket();
      LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
      LoRa.print(temp);
      LoRa.endPacket();    
      delay(1000);
    }
    userInput = "";
  }
      
}