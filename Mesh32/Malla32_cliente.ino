#include <WiFi.h>
#include "esp_wifi.h"
#include <time.h> 

int curChannel = 14;
const int ledPin = 2;

struct tm timeinfo;
struct timeval tv;
char strftime_buf[65];

time_t now;  

int ok_mac = 0;

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

extern "C" {
  esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);
//  esp_err_t esp_wifi_internal_set_rate(int a, int b, int c, wifi_internal_rate_t *d);
}

#define BEACON_SSID_OFFSET 38
#define SRCADDR_OFFSET 10
#define BSSID_OFFSET 16
#define SEQNUM_OFFSET 22
#define TOTAL_LINES (sizeof(rick_ssids) / sizeof(char *))


uint8_t beacon_raw[] = {
  0x80, 0x00,             // 0-1: Frame Control
  0x00, 0x00,             // 2-3: Duration
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,       // 4-9: Destination address (broadcast)
  0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06,       // 10-15: Source address
  0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06,       // 16-21: BSSID
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



void msg_task(char *msg) {

    uint8_t beacon_rick[200];
    memcpy(beacon_rick, beacon_raw, BEACON_SSID_OFFSET - 1);
    beacon_rick[BEACON_SSID_OFFSET - 1] = strlen(msg);
    memcpy(&beacon_rick[BEACON_SSID_OFFSET], msg, strlen(msg));
    memcpy(&beacon_rick[BEACON_SSID_OFFSET + strlen(msg)], &beacon_raw[BEACON_SSID_OFFSET], sizeof(beacon_raw) - BEACON_SSID_OFFSET);

    esp_wifi_80211_tx(WIFI_IF_AP, beacon_rick, sizeof(beacon_raw) + strlen(msg), false);

  
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
    ok_mac = 0;
  for(int i=0;i<=9;i++){ //   Serial.print(mac);Serial.print("-->");Serial.println(KnownMac[i][1]);
    
    if(mac == KnownMac[i][1]) {ok_mac = 1 ; break; }
    
    }

if (ok_mac == 1){
  
//Serial.println(packet);   

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

//mensaje.toCharArray(msg,mensaje.length());



Serial.println(mensaje);


char bssid[13];


for(int i=8;i<20;i++){  bssid[i-8] = packet[i];  } bssid[12] = '\0';
// Ajusta el valor now (epoch time) al enviado como MAC destino en el paquete beacon, generalmente FFFFFFFFFF
now = strtol(bssid, NULL, 16);
//Serial.print(now);
                                if (now > 1523537946) {
                                   
                                    digitalWrite(ledPin, millis()>>4 &1);
                                    tv.tv_sec=now;
                                   //tv.tv_usec=0;
                                   settimeofday(&tv,0);
                                   time(&now);
                                                      }
                                                      


//Serial.println(bssid);
//Serial.println(index);

   }

  } // Fin bucle receptor paquetes
  

//===== SETUP =====//
void setup() {

  /* start Serial */
  Serial.begin(9600);
  pinMode (ledPin, OUTPUT);
  digitalWrite (ledPin, LOW);
  /* setup wifi */
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);


  
  Serial.println("\n");
  
  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();
  
}

void loop() {

    delay(0);
    
   // digitalWrite (ledPin, LOW);

   
      //if (strlen(msg) != 0) {
        //msg_task(msg);
        //Serial.println(msg);
        //Serial.println(time(&now));
        
  // if (getLocalTime(&timeinfo)) {Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");}

   // }
    

 
    
}

