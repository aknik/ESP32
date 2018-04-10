#include <WiFi.h>
const char* ssid = "-------";
const char* password = "..";
 
/* Time Stamp */



const char* NTP_SERVER = "192.168.5.1";
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";

struct tm timeinfo;

void setup() {

Serial.begin(115200);
Serial.println("");
Serial.println("Time Stamp example");
Serial.println("");
Serial.print("Connecting to ");
Serial.println(ssid);
WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED)
{
delay(500);
Serial.print(".");
}
Serial.println("");
Serial.println("WiFi connected.");
Serial.println("IP address: ");
Serial.println(WiFi.localIP());

WLAN_2505
  configTzTime(TZ_INFO, NTP_SERVER);
  // start wifi here
  if (getLocalTime(&timeinfo, 10000)) {  // wait up to 10sec to sync
    Serial.println(&timeinfo, "Time set: %B %d %Y %H:%M:%S (%A)");
  } else {
    Serial.println("Time not set");
  }
}

void loop() {
  if (getLocalTime(&timeinfo)) {
    Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
  }
 delay(1000);
}








