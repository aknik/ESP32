#if defined(ESP32)
HardwareSerial Serial1(2);
#endif


#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"


#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_alloc_caps.h"


#include "driver/uart.h"
#include "minmea.h"


void setup() {
  Serial.begin(115200);
  Serial.println("GPS echo test");
  Serial1.begin(9600);      // default NMEA GPS baud
}

     
void loop() {

  if (Serial1.available()) {
    char c = Serial1.read();
    Serial.write(c);
  }
}
