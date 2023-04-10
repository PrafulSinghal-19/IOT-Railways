/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

// Load Wi-Fi library
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "heltec.h"
// Replace with your network credentials
const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";

#define BAND    868E6  //change this Band and set Class C

// const byte interruptPin = 0;
String message = "";
String id="1";

// Set web server port number to 80
WebServer server(80);


void handleNotFound() {
  message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.println("sent 404 on handleNotFound");
}

void handleOrder(){
  message = server.arg("plain");        
  server.send(200, "text/plain", "ORDER"+message);
  Serial.println("message: "+message);
}

void setup() {
  Serial.begin(115200);
  
  // pinMode(interruptPin, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(interruptPin), blink, FALLING);
  //WIFI Kit series V1 not support Vext control
  Heltec.begin(true , true , true , true , BAND );
  // int a = digitalRead(interruptPin);

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

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started "+id);
  
  Serial.println("id: "+id);
}

// void blink() {
//   Serial.println("Inside Blink");
// }

bool recv = true;

//While sending a msg only accept from those which have id less than yours.

void loop(){
  server.handleClient();
  // delay(2);//allow the cpu to switch to other tasks

  int packetSize = LoRa.parsePacket();
  String incoming = "";
  if (packetSize) {
    Serial.print("Received packet: ");
    while (LoRa.available()) {
      incoming+=(char)LoRa.read();
      Serial.print("w");
    }
    Serial.println("\nOut");
    Serial.println(id+" recieved "+incoming);
    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());
    if((incoming[0]-'0')<id.toInt()) recv = false;
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
  if(message!=""){
    Serial.println("inside message");
    LoRa.beginPacket();
    LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
    LoRa.print(message);
    LoRa.endPacket();   
    Serial.println(id+" sending "+message); 
    message = "";
  }
}
