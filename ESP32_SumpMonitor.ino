// set up ESP32 to only use single core for freeRTOS (simplifies setup)
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// variables
static const int led_pin = 3; // use GPIO3 pin for LED digital out

void toggleLED(void *parameter) {
  while(1) {
    digitalWrite(led_pin,HIGH);
    vTaskDelay(500/portTICK_PERIOD_MS);
    digitalWrite(led_pin,LOW);
    vTaskDelay(500/portTICK_PERIOD_MS);
  }
}

void setup () {
  pinMode(led_pin, OUTPUT);
  xTaskCreatePinnedToCore(  //use xTaskCreate() in Vanilla RTOS
              toggleLED,    //Function to be called
              "Toggle LED", //Name of taask
              1024,         //Stack size (bytes in ESP32 words in RTOS)
              NULL,         //Parameter to poass to function
              1,            //Task priority (0 to configMAX_PRIORITIES-1)
              NULL,         // Task handle
              app_cpu);    //run on one core for demo purposes (ESP32 only)
              // in vanilla RTOS, would also require vTaskStartScheduler() in main after setting up tasks
}

void loop () {
  //nothing needs to go here
}
