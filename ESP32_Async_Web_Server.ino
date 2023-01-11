// Import required libraries
#include "WiFi.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

// Replace with your network credentials
const char* ssid     = "SmithsDash";
const char* password = "";

// Stores LED state
String TOTALKM;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
 
String processor(const String& var)
{

  Serial.println(var);

  if(var == "TOTALKM")return "Hello world from placeholder!";
  if(var == "DAYKM")return "Hello world from placeholder!";
  if(var == "FUEL")return "Hello world from placeholder!";
  if(var == "AVERAGEFUEL")return "Hello world from placeholder!";
  if(var == "TOPSPEED")return "Hello world from placeholder!";
  if(var == "AVERAGEKMH")return "Hello world from placeholder!";

  return String();
}



void setup(){

// Serial port for debugging purposes
Serial.begin(115200);

if (!SPIFFS.begin(true)) {
Serial.println("An Error has occurred while mounting SPIFFS");
return;
}

File root = SPIFFS.open("/");

File filelist = root.openNextFile();

while(filelist){

Serial.print("FILE: ");
Serial.println(filelist.name());
filelist = root.openNextFile();
}

File file = SPIFFS.open("/log.txt", FILE_WRITE);

if (!file) {
Serial.println("There was an error opening the file for writing");
return;
}
if (file.print("TEST")) {
Serial.println("File was written");
} else {
Serial.println("File write failed");
}

file.close();



// Connect to Wi-Fi network with SSID and password
Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
WiFi.softAP(ssid, password);

IPAddress IP = WiFi.softAPIP();
Serial.print("AP IP address: ");
Serial.println(IP);

// Print ESP32 Local IP Address
Serial.println(WiFi.localIP());

// Route for root / web page
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
});

//Load PNG
server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/logo.png", "image/png");
  });

// Route for Home / web page
server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
});

// Route for download / web page
server.on("/download.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/download.html");
});

// Route for about / web page
server.on("/about.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/about.html");
});
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

 // Download Log
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/log.txt", "text/html", true);
  });

// Remove Log
  server.on("/remove", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS.remove("/log.txt"));
  });



  // Start server
  server.begin();
}
 
void loop(){
 
}
