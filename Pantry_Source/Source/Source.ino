#include "heltec.h"

// Load Wi-Fi library
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define BAND    868E6  //change this Band and set Class C

#define PANTRY "5"

const byte interruptPin = 0;
String userInput = "";
String id="4";


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

  Serial.println("Trying to send order...");
  String sendingMessage = id+" got and sent "+userInput;
  userInput=sendingMessage;
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
  
  // server.begin();
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
  if (Serial.available()) {
    userInput = Serial.readStringUntil('\n');
    userInput=id+' '+userInput+' '+id;
  }else{
    Serial.println("Serial not found");
  }
}
void blink() {
  getInput();
}

bool recv = true;

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
    Serial.println(id+" recieved "+incoming);
    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());
    if(incoming[0]-'0'<id.toInt())recv = false;
  }
  if(!recv){
    Serial.println("Trying to send...");
    String sendingMessage = id+" got and sent "+incoming;
    LoRa.beginPacket();
    LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
    LoRa.print(sendingMessage);
    LoRa.endPacket();
    Serial.println(id+" sending "+sendingMessage);
    recv = true;
  }
  if(userInput!=""){
    Serial.println("u: "+userInput);
    int len = userInput.length();

    for(int x = 0;x<=len/32;x++){
      String temp = userInput.substring((x*32),min((x+1)*32,len));
      LoRa.beginPacket();
      LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
      LoRa.print(temp);
      LoRa.endPacket();    
      delay(1000);
    }
    userInput = "";
  }
      
}