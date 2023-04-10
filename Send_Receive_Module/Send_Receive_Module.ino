#include "heltec.h"

#define BAND    868E6  //change this Band and set Class C

const byte interruptPin = 0;
String userInput = "";
String id="2";
void setup() {
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, FALLING);
  //WIFI Kit series V1 not support Vext control
  Heltec.begin(true , true , true , true , BAND );
  int a = digitalRead(interruptPin);
}
void getInput(){
  if (Serial.available()) {
    userInput = Serial.readStringUntil('\n');
    Serial.println("You entered: " + userInput);
    userInput=id+' '+userInput+' '+id;
  }else{
    Serial.println("Serial not found");
  }
}
void blink() {
  Serial.println("Inside Blink");
  // int a = digitalRead(interruptPin);
  getInput();
}

bool recv = true;

//While sending a msg only accept from those which have id less than yours.

void loop() {
  int packetSize = LoRa.parsePacket();
  String incoming = "";
  if (packetSize) {
    Serial.print("Received packet: ");
    while (LoRa.available()) {
      incoming+=(char)LoRa.read();
    }
    Serial.println(1+" recieved "+incoming);
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
    LoRa.beginPacket();
    LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
    LoRa.print(userInput);
    LoRa.endPacket();    
    userInput = "";
  }
      
}