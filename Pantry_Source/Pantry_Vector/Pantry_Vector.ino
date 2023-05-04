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
#define TIMEOUT 60*1000
#define PUSHED 5
String id="05";

Vector<Vector<String>> orders;
Vector<String> orderContainer[NUM_NODES];

String orderDisplayContainer[ORDER_DISPLAY];
Vector<String> orderDisplayed;

Vector<bool> pushedSourceId;
bool pushedSourceIdContainer[NUM_NODES];

unsigned long start;

int lineNumber = 0;

void setup() {
  //WIFI Kit series V1 not support Vext control
  orders.setStorage(orderContainer, NUM_NODES, NUM_NODES);
  orderDisplayed.setStorage(orderDisplayContainer,ORDER_DISPLAY,0);
  pushedSourceId.setStorage(pushedSourceIdContainer,NUM_NODES,NUM_NODES);

  Heltec.begin(true , true , true , true , BAND );
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0,0,id);
  Heltec.display->display();
  
  start = millis();

  Serial.println("id: "+id);
  Serial.println("size of orders: " + String(orders.size()));
}

void displayOrders(int nodeIndex){
  String jsonMessage = "";

  for(int i = 0; i < orders[nodeIndex].size() ; i++){
    jsonMessage += orders[nodeIndex][i];
  }

  if(jsonMessage!=""){
    Serial.println(jsonMessage);
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
      Heltec.display->drawString(0,lineNumber,name+" "+coach+" "+seat);  lineNumber+=11;
      Heltec.display->display();
      Serial.println("person: " + name + " ordering from coach: " + coach + " on seat: " + seat + " total price: " + totalPrice);
      JsonArray items = doc["items"];
      for (JsonVariant item : items) {
        String itemName = item["name"];    // Access "name" within each item
        double itemPrice = item["price"];     // Access "price" within each item
        int quantity = item["quantity"];
        Serial.println("item: " + itemName + " : " + String(itemPrice) + " quantity: " + quantity);
        Heltec.display->drawString(0,lineNumber,itemName+" "+quantity);  lineNumber+=11;
        Heltec.display->display();
      }
      
      orderDisplayed.push_back(coach+String(seat));
      Serial.println("pushed: "+String(orderDisplayed.size()));
    } else {
      Serial.println(jsonMessage);
      Serial.println("Parsing failed with error: " + String(error.c_str()));
    }
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

    if(incoming.substring(8) == "First"){
      if(orders[srcNodeIndex].size() == 0) {
        int count = incoming.substring(4,8).toInt();
        String* orderChunks = new String[count];
        orders[srcNodeIndex].setStorage(orderChunks, count, count);
        Serial.println("size of orders in srcNode: " + orders[srcNodeIndex].size());
      }
    }
    else if(incoming.substring(8) == "Push"){
      pushedSourceId[srcNodeIndex] = true;
    }
    else {
      int sequenceNo = incoming.substring(4,8).toInt();
      if(orders[srcNodeIndex][sequenceNo].length() == 0){
        orders[srcNodeIndex][sequenceNo] = incoming.substring(8);
      }
    }
    start = millis();
  }

  if(millis()-start>TIMEOUT){
    Heltec.display->clear();
    Heltec.display->display();
    lineNumber = 0;
    for(int x = 0;x<NUM_NODES;x++){
      displayOrders(x);
      orders[x].clear();
    }
    for(int x = 0;x<pushedSourceId.size();x++){
      if(pushedSourceId[x]){
        Serial.println(String(x+1)+" wants to order.");
        Heltec.display->drawString(0,lineNumber,String(x+1)+" wants to order.");  lineNumber+=11;
        Heltec.display->display();
        pushedSourceId[x] = false;
      }
    }
    Serial.println("Timeout reached clearing orders");
    start = millis();
    orderDisplayed.clear();
  }

}