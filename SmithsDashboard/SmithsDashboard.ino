// Import required libraries
#include "WiFi.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include <AsyncElegantOTA.h>;
#include "SPIFFS.h"
#include "FS.h"
#include <ArduinoJson.h>
#include <ESP_AVRISP.h>
#include <ESPmDNS.h>

// Replace with your network credentials
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";

const char *soft_ap_ssid          = "SMITHSDash";    /*Create a SSID for ESP32 Access Point*/
const char *soft_ap_password      = ""; /*Create Password for ESP32 AP*/

//ATMEL Update
const uint16_t port = 328;
const uint8_t reset_pin = 5;
ESP_AVRISP avrprog(port, reset_pin);

//Serial2
#define RXD2 16
#define TXD2 17

//Variables to save values from HTML form
String ssid;
String pass;
String ip;

// File paths to save input values permanently
const char* host = "esp-avrisp";
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";

//IPAddress localIP(192, 168, 1, 200); // hardcoded
IPAddress localIP;

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

// Initialize WiFi
bool initWiFi() {

  WiFi.mode(WIFI_AP_STA);
  
  // Connect to Wi-Fi network with SSID and password


    IPAddress IP = WiFi.softAPIP();
    Serial.println("Setting AP (Access Point)");
    WiFi.softAP(soft_ap_ssid, soft_ap_password);
    Serial.print("AP IP address: ");
    Serial.println(IP); 
    
  localIP.fromString(ip.c_str());

  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
    Serial.print(".");
    delay(100);
  }
  Serial.println();
  Serial.println(WiFi.localIP());
  return true;
}


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,void *arg, uint8_t *data, size_t len) {
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open("/log.txt", FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}
// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}
String processor(const String& var) {
  if (var == "FREESPIFFS") {
    return humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes()));
  }

  if (var == "USEDSPIFFS") {
    return humanReadableSize(SPIFFS.usedBytes());
  }

  if (var == "TOTALSPIFFS") {
    return humanReadableSize(SPIFFS.totalBytes());
  }

  return String();
}
void webserver() {

  // Route for root / web page
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false);
});

//Load PNG
server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/logo.png", "image/png");
  });

// Route for Home / web page
server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false);
});

// Route for download / web page
server.on("/download.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/download.html");
});

// Route for about / web page
server.on("/about.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/about.html", String(), false, processor);
});

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

 // Download Log
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/log.txt", String(), true);
  });

// Remove Log
  server.on("/remove", HTTP_GET, [](AsyncWebServerRequest *request){
    //request->send(SPIFFS.remove("/log.txt"));
    SPIFFS.remove("/log.txt");
    appendFile(SPIFFS, "/log.txt", "---------------------------------\r\n");
    appendFile(SPIFFS, "/log.txt", "--  SMITHSDash v.0.1  LogFile  --\r\n");
    appendFile(SPIFFS, "/log.txt", "--   by Ennar1991 & bripon76   --\r\n");
    appendFile(SPIFFS, "/log.txt", "---------------------------------\r\n");
    appendFile(SPIFFS, "/log.txt", "");
    request->send(SPIFFS, "/download.html");
  });

    server.serveStatic("/", SPIFFS, "/");

    server.on("/updatewifi", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "Done. SMITHSDash will restart, if connection failed: Connect to Wifi SMITHSDash and try to setup your settings again!");
      delay(3000);
      ESP.restart();
    });




  //Update Script
  AsyncElegantOTA.begin(&server);

  // Start server
  server.begin();
  
  }

void updateAtmel(){
    static AVRISPState_t last_state = AVRISP_STATE_IDLE;
    AVRISPState_t new_state = avrprog.update();
    if (last_state != new_state) {
        switch (new_state) {
            case AVRISP_STATE_IDLE: {
                Serial.printf("[AVRISP] now idle\r\n");
                // Use the SPI bus for other purposes
                break;
            }
            case AVRISP_STATE_PENDING: {
                Serial.printf("[AVRISP] connection pending\r\n");
                // Clean up your other purposes and prepare for programming mode
                break;
            }
            case AVRISP_STATE_ACTIVE: {
                Serial.printf("[AVRISP] programming mode\r\n");
                // Stand by for completion
                break;
            }
        }
        last_state = new_state;
    }
    // Serve the client
    if (last_state != AVRISP_STATE_IDLE) {
        avrprog.serve();
    }
  
  }


void setup(){

// Serial port for debugging purposes
Serial.begin(9600);
Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
initWebSocket();

  Serial.println("---------------------------------\r\n");
  Serial.println("--         SMITHSDash v.0.1    --\r\n");
  Serial.println("--   by Ennar1991 & bripon76   --\r\n");
  Serial.println("---------------------------------\r\n");


if (!SPIFFS.begin(true)) {
Serial.println("An Error has occurred while mounting SPIFFS");
return;
}

  Serial.print("SPIFFS Free: "); Serial.println(humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes())));
  Serial.print("SPIFFS Used: "); Serial.println(humanReadableSize(SPIFFS.usedBytes()));
  Serial.print("SPIFFS Total: "); Serial.println(humanReadableSize(SPIFFS.totalBytes()));


File root = SPIFFS.open("/");

File filelist = root.openNextFile();

while(filelist){

Serial.print("FILE: ");
Serial.println(filelist.name());
filelist = root.openNextFile();
}

  // Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  Serial.println(ssid);
  Serial.println(pass);

if(initWiFi()) {
webserver();
}
else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP(soft_ap_ssid, soft_ap_password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 
    webserver();
}

//AtmelUpdater
    Serial.println("");
    Serial.println("Arduino AVR-ISP over TCP");
    avrprog.setReset(false); // let the AVR run

    MDNS.begin(host);
    MDNS.addService("avrisp", "tcp", port);

    IPAddress local_ip = WiFi.localIP();
    Serial.print("IP address: ");
    Serial.println(local_ip);
    Serial.println("Use your avrdude:");
    Serial.print("avrdude -c arduino -p <device> -P net:");
    Serial.print(local_ip);
    Serial.print(":");
    Serial.print(port);
    Serial.println(" -t # or -U ...");

    // listen for avrdudes
    avrprog.begin();

}

char serialBuffer[1000];
char serialBufferPosition=0;


void loop(){

updateAtmel();
ws.cleanupClients();

  if (Serial2.available()) {
      serialBuffer[serialBufferPosition]=Serial2.read();
      if (serialBuffer[serialBufferPosition]=='\n') {
        //do some kind of action
          ws.textAll(String(serialBuffer)); //this is what we send to the browser
          //appendFile(SPIFFS, "/log.txt", (serialBuffer));
          Serial.println(serialBuffer);
        serialBufferPosition=0;
        memset(serialBuffer, 0, sizeof(serialBuffer));
      } else{
        serialBufferPosition++;
      }
  }


  } 
