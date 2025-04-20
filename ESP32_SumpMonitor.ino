// set up ESP32 to only use single core for freeRTOS (simplifies setup)
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
 #include <AsyncEventSource.h>

#define WIFI_SSID "NETGEAR27"
// Defining the WiFi channel speeds up the connection:
#define WIFI_CHANNEL 6
#define WL_MAX_ATTEMPT_CONNECTION 10
#define PUMP_ON 0   // energized relay turns pump on 
#define PUMP_OFF 1  // de-energized relay will turn pump off (default)

AsyncWebServer server(80);
AsyncEventSource events("/events");

const int SWITCH = 17;
const int RELAY = 19;
const int LED = 2;

String WIFI_PASSWORD = "";
int wifiTimeout = 0;

volatile bool SWITCHState = false;
bool RELAYState = PUMP_OFF;
bool inputReceived = false;
bool pumpHysteretic = 0; 

portMUX_TYPE switchMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR switchISR() {
  portENTER_CRITICAL_ISR(&switchMux);
  SWITCHState = !SWITCHState;
  portEXIT_CRITICAL_ISR(&switchMux);
}

void handleSwitchTask (void *pvParameters) {
  bool lastDebouncedState = digitalRead(SWITCH);
  unsigned long pumpStartTime = 0;

  while (1) {
   vTaskDelay(pdMS_TO_TICKS(100));
   bool currentState = digitalRead(SWITCH);

   if (currentState != lastDebouncedState) {
     lastDebouncedState = currentState;
     portENTER_CRITICAL(&switchMux);
     SWITCHState = currentState; // Update SWITCHState based on debounced input
     portEXIT_CRITICAL(&switchMux);

     if (!currentState) {                   // if switch on but pump not on
       digitalWrite(RELAY, PUMP_ON);        // turn pump on
       pumpHysteretic = 1;                      // update pump status
       events.send("ON","switchStatus");   // send SSE event for switch ON
       events.send("ON","pumpStatus");      // send SSE event for pump ON
     } else if (currentState) {             // if switch off
      pumpStartTime = millis();             // start pump timer 
      events.send("OFF", "switchStatus");   // send SSE event for switch OFF
     }
   }

   // Check if pump-on duration has elapsed
   if (currentState && pumpHysteretic && (millis() - pumpStartTime >= 5000)) {
     digitalWrite(RELAY, PUMP_OFF);
     pumpHysteretic = 0;
     events.send("OFF", "pumpStatus");
   }
  }
 }

void toggleLED(void *parameter) {
  while(1) {
    digitalWrite(LED,HIGH);
    vTaskDelay(500/portTICK_PERIOD_MS);
    digitalWrite(LED,LOW);
    vTaskDelay(500/portTICK_PERIOD_MS);
  }
}

String getHtml() {
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
         .status-box {
           padding: 0.5em 1em;
           font-size: 2em;
           border: 1px solid #ccc; /* Optional border */
         }
         .status-on {
           background-color: #5B5;
           color: #fff;
         }
         .status-off {
           background-color: #333;
           color: #fff;
         }
         .btn { background-color: #5B5; border: none; color: #fff; padding: 0.5em 1em;
          font-size: 2em; text-decoration: none; 
        } 
         .btn.OFF { background-color: #333; }
       </style>
     </head>

     <body>
       <h1>ESP32 Web Server</h1>

       <div>
         <h2>Switch Status</h2>
         <span id="switchStatus" class="status-box SWITCH_CLASS">SWITCH_TEXT</span>
         <h2>Pump Motor</h2>
         <a href="/toggle/2" class="btn RELAY_TEXT">RELAY_TEXT</a>
       </div>

       <script>
         if (!!window.EventSource) {
           var source = new EventSource('/events');

           source.addEventListener('switchStatus', function(e) {
             var switchStatusElement = document.getElementById('switchStatus');
             switchStatusElement.innerText = e.data === 'ON' ? 'ON' : 'OFF';
             switchStatusElement.className = 'status-box ' + (e.data === 'ON' ? 'status-on' : 'status-off');
           }, false);

           source.addEventListener('pumpStatus', function(e) {
             var relayButton = document.querySelector('.relayButton');
             relayButton.innerText = 'Pump Motor: ' + (e.data === 'ON' ? 'ON' : 'OFF');
             relayButton.className = 'btn ' + (e.data === 'ON' ? '' : 'OFF');
           }, false);

           source.addEventListener('error', function(event) {
             if (event.target.readyState === EventSource.CLOSED) {
               console.log('EventSource disconnected');
             } else if (event.target.readyState === EventSource.CONNECTING) {
               console.log('EventSource connecting...');
             }
           }, false);
         } else {
           console.log("Your browser doesn't support SSE");
         }
       </script>
       
     </body>
   </html>
  )";
  response.replace("SWITCH_TEXT", SWITCHState ? "ON" : "OFF");
  response.replace("RELAY_TEXT", RELAYState ? "OFF" : "ON");
  response.replace("SWITCH_CLASS", SWITCHState ? "status-on" : "status-off");
  return response;
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
  pinMode(SWITCH, INPUT_PULLUP);
  pinMode(RELAY, OUTPUT);
  pinMode(LED,OUTPUT);
  digitalWrite(RELAY, PUMP_OFF);

  beginWifi();

  xTaskCreatePinnedToCore(  //use xTaskCreate() in Vanilla RTOS
    toggleLED,    //Function to be called
    "Toggle LED", //Name of taask
    1024,         //Stack size (bytes in ESP32 words in RTOS)
    NULL,         //Parameter to poass to function
    1,            //Task priority (0 to configMAX_PRIORITIES-1)
    NULL,         // Task handle
    app_cpu);    //run on one core for demo purposes (ESP32 only)
    // in vanilla RTOS, would also require vTaskStartScheduler() in main after setting up tasks

    xTaskCreatePinnedToCore(
      handleSwitchTask,   /* Task function */
      "HandleSwitch",     /* Name of task */
      4096,               /* Stack size (adjust as needed) */
      NULL,               /* Parameter passed to task */
      2,                  /* Task priority (adjust as needed) */
      NULL,               /* Task handle */
      app_cpu);           /* Core to run on */

    attachInterrupt(digitalPinToInterrupt(SWITCH), switchISR, CHANGE);

  server.on("/", [](AsyncWebServerRequest *request){
    request->send(200, "text/html", getHtml());
   });

  server.on("/toggle/2", [](AsyncWebServerRequest *request) {
    RELAYState = !RELAYState;
    digitalWrite(RELAY,RELAYState);
    request->send(200, "text/html", getHtml());
  });

  server.addHandler(&events); // Add the event source handler
  server.begin();
  Serial.println("HTTP server started");

}

void loop(void) {
  //allow FreeRTOS to handle tasks
}