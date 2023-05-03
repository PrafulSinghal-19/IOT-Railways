#include "heltec.h"
#include "ArduinoJson.h"

/* 
First:
<prev_src> 2
<src-id> 2
<length> 4
<message> "First"

Next:
<prev_src> 2
<src> 2
<seq> 4
<message> ...
*/
#define BAND    868E6  //change this Band and set Class C
#define PANTRY "5"
String id="05";

void setup() {
  //WIFI Kit series V1 not support Vext control
  Heltec.begin(true , true , true , true , BAND );
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_10);
  Serial.println("id: "+id);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  String incoming = "";
  String jsonMessage = "";
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
      int packetCount = 0;
      jsonMessage = "";
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
        jsonMessage += temp.substring(8);
        packetCount = packetCount + 1;
        if(temp.substring(4,8).toInt() == count -1) break; // check for last packet if some packet missed
      }

      Serial.println("received json message: " + jsonMessage);
      DynamicJsonDocument doc(jsonMessage.length()*10);
      DeserializationError error = deserializeJson(doc, jsonMessage);

      if (!error) {
        String name = doc["name"];  // Access "name"
        String coach = doc["coach"];       // Access "coach"
        int seat = doc["seat"];
        double totalPrice = doc["totalPrice"];
        Serial.println("person: " + name + " ordering from coach: " + coach + " on seat: " + seat + " total price: " + totalPrice);
        JsonArray items = doc["items"];
        for (JsonVariant item : items) {
          String itemName = item["name"];    // Access "name" within each item
          double itemPrice = item["price"];     // Access "price" within each item
          int quantity = item["quantity"];
          Serial.println("item: " + itemName + " : " + String(itemPrice) + " quantity: " + quantity);
        }
      } else {
        Serial.println("Parsing failed with error: " + String(error.c_str()));
      }
    }
  }
}