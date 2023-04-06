/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

// Load Wi-Fi library
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
// Replace with your network credentials
const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";

// Set web server port number to 80
WebServer server(80);

// Variable to store the HTTP request
String header;

void handleNotFound() {
  String message = "File Not Found\n\n";
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
  String message = server.arg("plain");        
  server.send(200, "text/plain", "ORDER"+message);
  Serial.println("message: "+message);
}

void setup() {
  Serial.begin(115200);

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
  Serial.println("HTTP server started");
  
}

void loop(){
  server.handleClient();
  delay(2);//allow the cpu to switch to other tasks
}
