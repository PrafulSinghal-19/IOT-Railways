#include "heltec.h"
#include "ArduinoJson.h"
#include "Vector.h"

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
#define NUM_NODES 4
#define ORDER_DISPLAY 20
String id="05";

Vector<Vector<String>> orders;
Vector<int> packetsLeft;
int packetsLeftContainer[NUM_NODES];
Vector<String> orderContainer[NUM_NODES];

String orderDisplayContainer[ORDER_DISPLAY];
Vector<String> orderDisplayed;


void setup() {
  //WIFI Kit series V1 not support Vext control
  packetsLeft.setStorage(packetsLeftContainer, NUM_NODES, NUM_NODES);
  orders.setStorage(orderContainer, NUM_NODES, NUM_NODES);
  orderDisplayed.setStorage(orderDisplayContainer,ORDER_DISPLAY,0);
  Heltec.begin(true , true , true , true , BAND );
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_10);
  
  Serial.println("id: "+id);
  Serial.println("size of orders: " + String(orders.size()));
  Serial.println("size of packetsLeft array: " + String(packetsLeft.size()));
}

void displayOrders(int nodeIndex){

  Serial.println("message completely recieved for node: " + String(nodeIndex));
  String jsonMessage = "";

  for(int i = 0; i < orders[nodeIndex].size() ; i++){
    jsonMessage += orders[nodeIndex][i];
  }

  DynamicJsonDocument doc(jsonMessage.length()*10);
  DeserializationError error = deserializeJson(doc, jsonMessage);

  if (!error) {
    String name = doc["name"];  // Access "name"
    String coach = doc["coach"];       // Access "coach"
    int seat = doc["seat"];
    double totalPrice = doc["totalPrice"];

    for(auto orderId: orderDisplayed){
      if(orderId==coach+String(seat)){
        return;
      }
    }
    Serial.println("person: " + name + " ordering from coach: " + coach + " on seat: " + seat + " total price: " + totalPrice);
    JsonArray items = doc["items"];
    for (JsonVariant item : items) {
      String itemName = item["name"];    // Access "name" within each item
      double itemPrice = item["price"];     // Access "price" within each item
      int quantity = item["quantity"];
      Serial.println("item: " + itemName + " : " + String(itemPrice) + " quantity: " + quantity);
    }
    
    orderDisplayed.push_back(coach+String(seat));
    Serial.println("pushed: "+String(orderDisplayed.size()));
  } else {
    Serial.println(jsonMessage);
    Serial.println("Parsing failed with error: " + String(error.c_str()));
  }
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
    int srcNodeIndex = incoming.substring(2,4).toInt() - 1;
    Serial.println("recieved from node index: " + String(srcNodeIndex));
    Serial.println(id+" received "+incoming);
    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());

    if(incoming.substring(8) == "First"){
      if(packetsLeft[srcNodeIndex] == 0) {
        int count = incoming.substring(4,8).toInt();
        packetsLeft[srcNodeIndex] = count;
        String* orderChunks = new String[count];
        orders[srcNodeIndex].setStorage(orderChunks, count, count);
        Serial.println("size of orders in srcNode: " + orders[srcNodeIndex].size());
      }
    }
    else {
      int sequenceNo = incoming.substring(4,8).toInt();
      if(orders[srcNodeIndex][sequenceNo].length() == 0){
        orders[srcNodeIndex][sequenceNo] = incoming.substring(8);
        packetsLeft[srcNodeIndex] = packetsLeft[srcNodeIndex] - 1;
      }
      if(packetsLeft[srcNodeIndex] == 0){
        displayOrders(srcNodeIndex);
      } 
    }
  }
}