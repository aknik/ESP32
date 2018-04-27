#include <WiFi.h>
#include "esp_wifi.h"
#include <time.h>
#include <sys/time.h>

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// UART driver
#include "driver/uart.h"
// Error library
#include "esp_err.h"
// minmea
#include "minmea.h"
#define MAX_LINE_SIZE 255


uint8_t line[MAX_LINE_SIZE+1];
  
const char* ssid = "WLAN_2505";
const char* password = ".999999999.";
int contador = 0 ;
  
struct tm timeinfo;
struct timeval tv;
time_t now;  

char strftime_buf[32];
const int ledPin = 2;

int curChannel = 14;

String KnownMac[10][2] = {  // MAC MAYUSCULAS !!!!!!!
  {"Will-Phone","515151515151"},
  {"Will-PC","BADEAFFE0006"},
  {"NAME","BADEAFFE0007"},
  {"NAME","MACADDRESS"}
  
};

String defaultTTL = "60"; // Maximum time (Apx seconds) elapsed before device is consirded offline

const wifi_promiscuous_filter_t filt={ //Idk what this does
    .filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT|WIFI_PROMIS_FILTER_MASK_DATA
};

typedef struct { // or this
  uint8_t mac[6];
} __attribute__((packed)) MacAddr;

typedef struct { // still dont know much about this
  int16_t fctl;
  int16_t duration;
  MacAddr da;
  MacAddr sa;
  MacAddr bssid;
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;

/////////////////////////////////////////////////////////////////////////
char* read_line(uart_port_t uart_controller) {

  static char line[MINMEA_MAX_LENGTH];
  char *ptr = line;
  while(1) {
  
    int num_read = uart_read_bytes(uart_controller, (unsigned char *)ptr, 1, portMAX_DELAY);
    if(num_read == 1) {
    
      // new line found, terminate the string and return
      if(*ptr == '\n') {
        ptr++;
        *ptr = '\0';
        return line;
      }
      
      // else move to the next char
      ptr++;
    }
  }
}


/////////////////////////////////////////////////////////////////////////
extern "C" {
  esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);
//  esp_err_t esp_wifi_internal_set_rate(int a, int b, int c, wifi_internal_rate_t *d);
}


#define BEACON_SSID_OFFSET 38
#define SRCADDR_OFFSET 10
#define BSSID_OFFSET 16
#define SEQNUM_OFFSET 22

#define DES_OFFSET 4

uint8_t beacon_raw[] = {
  0x80, 0x00,             // 0-1: Frame Control
  0x00, 0x00,             // 2-3: Duration
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,       // 4-9: Destination address (broadcast)
  0xba, 0xde, 0xaf, 0xfe, 0x00, 0x07,       // 10-15: Source address
  0xba, 0xde, 0xaf, 0xfe, 0x00, 0x07,       // 16-21: BSSID
  0x00, 0x00,             // 22-23: Sequence / fragment number
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,     // 24-31: Timestamp (GETS OVERWRITTEN TO 0 BY HARDWARE)
  0x64, 0x00,             // 32-33: Beacon interval
  0x31, 0x04,             // 34-35: Capability info
  0x00, 0x00, /* FILL CONTENT HERE */       // 36-38: SSID parameter set, 0x00:length:content
  0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, // 39-48: Supported rates
  0x03, 0x01, 0x0e,           // 49-51: DS Parameter set, current channel 1 (= 0x01),
  0x05, 0x04, 0x01, 0x02, 0x00, 0x00,       // 52-57: Traffic Indication Map
  
};

int StrToHex(char str[])
{
  return (uint8_t) strtol(str, 0, 16);
}





void msg_task(char *msg, char *bssid) {

char temp[3];
int j = 5;

for(int i=11;i>=1;i=i-2){ 

    temp[0] = bssid[i-1];
    temp[1] = bssid[i] ;
    temp[2] = '\0';
    beacon_raw[4+j] = StrToHex(temp);
    //Serial.print(i);Serial.print(" ");
    //Serial.println(bssid[i],HEX);
    
    j=j-1;
    
  
}
//Serial.println("_");

    uint8_t beacon_rick[200];
    memcpy(beacon_rick, beacon_raw, BEACON_SSID_OFFSET - 1);
    beacon_rick[BEACON_SSID_OFFSET - 1] = strlen(msg);
    memcpy(&beacon_rick[BEACON_SSID_OFFSET], msg, strlen(msg));
    memcpy(&beacon_rick[BEACON_SSID_OFFSET + strlen(msg)], &beacon_raw[BEACON_SSID_OFFSET], sizeof(beacon_raw) - BEACON_SSID_OFFSET);

    esp_wifi_80211_tx(WIFI_IF_AP, beacon_rick, sizeof(beacon_raw) + strlen(msg), false);
/*   
    String hexd;
    for(int i=0;i<200;i++){ 
     hexd = String(beacon_rick[i],HEX); 
     
     if (hexd.length() < 2) hexd = "0" + hexd;
     Serial.print(hexd);
         
  }
  Serial.println("");
  */
}

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) { //This is where packets end up after they get sniffed
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf; // Dont know what these 3 lines do
  
  int len = p->rx_ctrl.sig_len;
  WifiMgmtHdr *wh = (WifiMgmtHdr*)p->payload;
  len -= sizeof(WifiMgmtHdr);
  if (len < 0){
    Serial.println("Receuved 0");
    return;
  }
  String packet;
  String hexd;
  String mac;
  int fctl = ntohs(wh->fctl);
  //for(int i=8;i<=8+6+1;i++){ // This reads the first couple of bytes of the packet. This is where you can read the whole packet replaceing the "8+6+1" with "p->rx_ctrl.sig_len"
    for(int i=0;i<=p->rx_ctrl.sig_len;i++){ 
     hexd = String(p->payload[i],HEX); 
     if (hexd.length() < 2) hexd = "0" + hexd;
     //Serial.print(hexd);
     packet += hexd;
     
  }
  
  for(int i=20;i<=31;i++){  mac += packet[i];  }  mac.toUpperCase();
  int ok_mac = 0;
  for(int i=0;i<=9;i++){ //   Serial.print(mac);Serial.print("-->");Serial.println(KnownMac[i][1]);
    
    if(mac == KnownMac[i][1]) {ok_mac = 1 ; break; }
    //Serial.println(packet);    
    }

if (ok_mac == 1){

char msg[33];   
String mensaje ; 
char temp[3];
long index;
len = p->payload[37]+1;   

// Leemos el SSID del beacon, p->payload[37] guarda la longitud del campo SSID, el nombre de la red Wifi :-)

      for (int i = 0; i < len*2 ; i += 2) {
      char c;
      temp[0] = packet[76+i];
      temp[1] = packet[77+i];
      temp[2] = '\0';
      index = strtol(temp, NULL, 16);
      //if ( (index > 47) && (index < 58) ){
      c = toascii(index);
      mensaje = mensaje + c;
                                          }

// mensaje = mensaje + "L";
//strcpy(msg,"\0");

mensaje.toCharArray(msg,mensaje.length());

Serial.print(">> ");
Serial.println(msg);

        /*/ Lee la MAC destino del paquete ...
                        char bssid[13];
                        for(int i=8;i<20;i++){  bssid[i-8] = packet[i];  } bssid[12] = '\0';
                        // Ajusta el valor now (epoch time) al enviado como MAC destino en el paquete beacon, generalmente FFFFFFFFFF
                        now = strtol(bssid, NULL, 16);

                                if (now > 1523537946) {
                                   
                                    digitalWrite(ledPin, millis()>>4 &1);
                                    tv.tv_sec=now;
                                   //tv.tv_usec=0;
                                   settimeofday(&tv,0);
                                   time(&now);
                                                      }
//Serial.println(bssid);
//Serial.println(index);

*/

   }

  } // Fin bucle receptor paquetes

const char* NTP_SERVER = "192.168.5.1";
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";



  
//===== SETUP =====//
void setup() {
  
/* start Serial */
Serial.begin(115200);
pinMode (ledPin, OUTPUT);
digitalWrite (ledPin, LOW);
/*
Serial.println("");
Serial.println("Time Stamp example");
Serial.println("");
Serial.print("Connecting to ");
Serial.println(ssid);
WiFi.begin(ssid, password);

contador = 0 ;
while (WiFi.status() != WL_CONNECTED)
{
delay(500);
contador = contador + 1 ;
if ( contador > 20 ) ESP.restart();
Serial.print(".");

}
Serial.println("");
Serial.println("WiFi connected.");
Serial.println("IP address: ");
Serial.println(WiFi.localIP());


  configTzTime(TZ_INFO, NTP_SERVER);
  // start wifi here
  if (getLocalTime(&timeinfo, 10000)) {  // wait up to 10sec to sync
    Serial.println(&timeinfo, "Time set: %B %d %Y %H:%M:%S (%A)");
  } else {
    Serial.println("Time not set");
  }

delay(5000);
contador = 0 ;
  
*/

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);



}


void antiloop() {
  
tv.tv_sec = time(&now);
localtime_r(&now, &timeinfo);
strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
//Serial.println(strftime_buf);

unsigned long epoch = time(&now);

char bssid[13];
char ceros[13];
char combined[13];
strcpy(combined,"\0");
strcpy(ceros,"\0");
strcpy(bssid,"\0");

ultoa(epoch, bssid, 16);

//Serial.println(strlen(bssid));
//Serial.println(bssid);


strncpy ( ceros, "000000000000", 12-strlen(bssid) );

strcat(combined, ceros);
strcat(combined, bssid);


if (strlen(strftime_buf) > 1) {
        msg_task(strftime_buf,combined);
        Serial.print(strftime_buf);Serial.print("          ");Serial.println(combined);
                                }
 strcpy(strftime_buf,"\0");   
 delay(1000);

contador = contador + 1 ;
if ( contador > 1000 ) ESP.restart();
    
}



void loop() {

  
  // configure the UART1 controller, connected to the GPS receiver
  uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, 4, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0);
  

  
  // parse any incoming messages and print it
  while(1) {
    
    // read a line from the receiver
    char *line = read_line(UART_NUM_1);


    ///////////////Serial.print(line);
    
    switch (minmea_sentence_id(line, false)) {
      
            case MINMEA_SENTENCE_RMC: {
                
        struct minmea_sentence_rmc frame;
        
    if (minmea_parse_rmc(&frame, line)) {
      
        timeinfo.tm_hour = frame.time.hours ;
        timeinfo.tm_min = frame.time.minutes;
        timeinfo.tm_sec = frame.time.seconds ;
        timeinfo.tm_mday = frame.date.day;
        timeinfo.tm_mon = frame.date.month-1;
        timeinfo.tm_year = frame.date.year+100;


        time_t now =  mktime(&timeinfo);
        
// printf("\nTime: %d:%d:%d %d-%d-%d",frame.time.hours, frame.time.minutes, frame.time.seconds, frame.date.day, frame.date.month, frame.date.year);
        
        //Serial.println(t);
        // settimeofday(&tv, 0); 
              
tv.tv_sec = now ;
localtime_r(&now, &timeinfo);
        settimeofday(&tv, 0); 
        time(&now);
strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
//Serial.println(strftime_buf);

unsigned long epoch = time(&now);
Serial.println(epoch);

char bssid[13];
char ceros[13];
char combined[13];
strcpy(combined,"\0");
strcpy(ceros,"\0");
strcpy(bssid,"\0");

ultoa(epoch, bssid, 16);

//Serial.println(strlen(bssid));
//Serial.println(bssid);


strncpy ( ceros, "000000000000", 12-strlen(bssid) );

strcat(combined, ceros);
strcat(combined, bssid);



String linea(line) ;



// linea = "$GPRMC,190956.00,A,3916.25172,N,00219.23703,W,0.621,,260418,,,A*6D";



if (linea.indexOf("V") == -1) {

  String nmea = linea.substring(19, 28) + linea.charAt(30) + " " +linea.substring(32,42) + linea.charAt(44); 
  
  //linea.toCharArray(strftime_buf,32);

msg_task(line,combined);  


Serial.println(nmea);













   
    
                            }

        
                          } 
                  
          
                   } 

  

      
      
        }
    }

}
void printPart(char* txt, byte start, byte len){
  for(byte i = 0; i < len; i++){
    Serial.write(txt[start + i]);
    //same as
    //Serial.print(*(txt + start + i));
  } 
}
