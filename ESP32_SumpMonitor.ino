#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>

#define WIFI_SSID "NETGEAR27"
// Defining the WiFi channel speeds up the connection:
#define WIFI_CHANNEL 6
#define WL_MAX_ATTEMPT_CONNECTION 10
#define PUMP_ON 0   // energized relay turns pump on 
#define PUMP_OFF 1  // de-energized relay will turn pump off (default)

AsyncWebServer server(80);

const int SWITCH = 17;
const int RELAY = 19;

String WIFI_PASSWORD = "";
int wifiTimeout = 0;

bool SWITCHState = false;
bool RELAYState = PUMP_OFF;
bool inputReceived = false;

void getHtml() {
  String response = R"(
    <!DOCTYPE html><html>
      <head>
        <title>ESP32 Web Server Demo</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html { font-family: sans-serif; text-align: center; }
          body { display: inline-flex; flex-direction: column; }
          h1 { margin-bottom: 1.2em; } 
          h2 { margin: 0; }
          div { display: grid; grid-template-columns: 1fr 1fr; grid-template-rows: auto auto; grid-auto-flow: column; grid-gap: 1em; }
          .btn { background-color: #5B5; border: none; color: #fff; padding: 0.5em 1em;
                 font-size: 2em; text-decoration: none }
          .btn.OFF { background-color: #333; }
        </style>
      </head>
            
      <body>
        <h1>ESP32 Web Server</h1>

        <div>
          <h2>Switch Status</h2>
          <a href="/toggle/1" class="btn SWITCH_TEXT">SWITCH_TEXT</a>
          <h2>Pump Motor</h2>
          <a href="/toggle/2" class="btn RELAY_TEXT">RELAY_TEXT</a>
        </div>
      </body>
    </html>
  )";
  response.replace("SWITCH_TEXT", SWITCHState ? "ON" : "OFF");
  response.replace("RELAY_TEXT", RELAYState ? "OFF" : "ON");
  server.send(200, "text/html", response);
}



String readFullStringBlocking() {
  String inputString = "";
  bool stringComplete = false;

//  Serial.println("Enter a text string and press Enter:");

  while (!stringComplete) {
    if (Serial.available() > 0) {
      char inChar = (char)Serial.read();
      if (inChar == '\n') {
        stringComplete = true;
        inputString.trim(); // Remove any trailing newline or carriage return
      } else if (inChar != '\r') { // Ignore carriage return
        inputString += inChar;
      }
    }
    // Optionally add a small delay to prevent busy-waiting
    delay(10);
  }
  return inputString;
}

void beginWifi(){
    esp_log_level_set("wifi", ESP_LOG_NONE); //set logging to none to avoid unneccessary print statements
    while (WiFi.status() != WL_CONNECTED) {
    inputReceived = 0;
    wifiTimeout = 0;
    Serial.print("Enter password to connect to network ");
    Serial.println(WIFI_SSID);
    while (!inputReceived) {
        WIFI_PASSWORD = readFullStringBlocking();
        inputReceived = true;
        WIFI_PASSWORD.trim(); // Remove any trailing newline or carriage return
    }
    Serial.print("Connecting to WiFi ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
    // Wait for connection
    while (WiFi.status()!= WL_CONNECTED & wifiTimeout<10 ) {
      delay(1000);
      Serial.print(".");
      wifiTimeout +=1;
    }
    Serial.println("");
  }
  Serial.println(" Connected!");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}



void setup(void) {
  Serial.begin(115200);
  
  beginWifi();

  server.on("/", [](AsyncWebServerRequest *request){
    request->send(200, "text/html", getHtml());
   });
 
  server.on("/toggle/2", [](AsyncWebServerRequest *request){
    SWITCHState = digitalRead(SWITCH);
    request->send(200, "text/html", getHtml());
   });

  server.on("/toggle/1", [](AsyncWebServerRequest *request) {
    RELAYState = !RELAYState;
    digitalWrite(RELAY,RELAYState);
    request->send(200, "text/html", getHtml());
  });
 
  server.begin();
  Serial.println("HTTP server started");

  pinMode(SWITCH, INPUT_PULLDOWN);
  pinMode(RELAY, OUTPUT);
}

void loop(void) {
  delay(2);
}
