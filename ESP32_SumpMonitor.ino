#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>

#define WIFI_SSID "NETGEAR27"
// Defining the WiFi channel speeds up the connection:
#define WIFI_CHANNEL 6
#define WL_MAX_ATTEMPT_CONNECTION 10

WebServer server(80);

const int LED1 = 2;
const int LED2 = 3;

String WIFI_PASSWORD = "";
int wifiTimeout = 0;

bool led1State = false;
bool led2State = false;
bool inputReceived = false;

void sendHtml() {
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
          <h2>LED 1</h2>
          <a href="/toggle/1" class="btn LED1_TEXT">LED1_TEXT</a>
          <h2>LED 2</h2>
          <a href="/toggle/2" class="btn LED2_TEXT">LED2_TEXT</a>
        </div>
      </body>git st
    </html>
  )";
  response.replace("LED1_TEXT", led1State ? "ON" : "OFF");
  response.replace("LED2_TEXT", led2State ? "ON" : "OFF");
  server.send(200, "text/html", response);
}

String readFullStringBlocking() {
  String inputString = "";
  bool stringComplete = false;

  Serial.println("Enter a text string and press Enter:");

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



void setup(void) {
  Serial.begin(115200);
  esp_log_level_set("wifi", ESP_LOG_NONE); //set logging to none to avoid unneccessary print statements
  while (WiFi.status() != WL_CONNECTED) {
    inputReceived = 0;
    wifiTimeout = 0;
    Serial.print("Enter password to connect to network ");
    Serial.println(WIFI_SSID);
    while (!inputReceived) {
      //if (Serial.available() > 0) {
        Serial.println("reading inputs");
        WIFI_PASSWORD = readFullStringBlocking();
        Serial.print("recieved: ");
        Serial.println(WIFI_PASSWORD);
        Serial.println("done reading inputs");
        inputReceived = true;
        WIFI_PASSWORD.trim(); // Remove any trailing newline or carriage return
        //}
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

  server.on("/", sendHtml);

  server.on(UriBraces("/toggle/{}"), []() {
    String led = server.pathArg(0);
    Serial.print("Toggle LED #");
    Serial.println(led);

    switch (led.toInt()) {
      case 1:
        led1State = !led1State;
        digitalWrite(LED1, led1State);
        break;
      case 2:
        led2State = !led2State;
        digitalWrite(LED2, led2State);
        break;
    }

    sendHtml();
  });

  server.begin();
  Serial.println("HTTP server started");

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
}

void loop(void) {
  server.handleClient();
  delay(2);
}
