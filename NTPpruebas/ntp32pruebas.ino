#include <WiFi.h>
#include <time.h>

#include <lwip/sockets.h>
#include <apps/sntp/sntp.h>

const char* ssid = "WLAN";
const char* password = "..";

struct tm timeinfo;
struct timeval tv;
time_t now;
int contador = 0;
char strftime_buf[64];

void setup() {

Serial.begin(115200);
Serial.println("");
Serial.println("Time NTP");
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

  ip_addr_t addr;
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  inet_pton(AF_INET, "192.168.5.1", &addr); sntp_setserver(0, &addr); 
  // Si usamos direccion ip , comentar la linea siguente y descomentar la anterior. 
  //sntp_setservername(0, "hora.roa.es");
}

void loop() {
  
tv.tv_sec = time(&now);
localtime_r(&now, &timeinfo);
strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

if ((now < 10000) || (contador > 60)){  Serial.print("-- Consulta al servidor NTP ---  ");  updatetime();   }
else {

Serial.println(strftime_buf);
contador = contador + 1;
}

delay(990);

}

void updatetime(){
  
  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();

  sntp_init();

  contador = 0;
  Serial.println(now);
  
  }
