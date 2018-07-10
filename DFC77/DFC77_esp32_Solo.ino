#include <WiFi.h>
#include <Ticker.h>
#include <Time.h>

const char* ssid = "MI_WIFI";
const char* password = "........";

Ticker tickerSetLow;

const int ledPin = 2;

//complete array of pulses for three minutes
//0 = no pulse, 1=100msec, 2=200msec
int ArrayImpulsi[60];
int ContaImpulsiParziale = 0;
int Ore,Minuti,Secondi,Giorno,Mese,Anno,DayOfW;
const int timeZone = 1;     // Central European Time

int Dls;                    //DayLightSaving
// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).7750000ULL


time_t epoch ;

const char* NTP_SERVER = "es.pool.ntp.org";
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";

struct tm timeinfo;
struct timeval tv;
char strftime_buf[65];
long unsigned trabajo,dfc77 ;


void setup(){

  
  Serial.begin(115200);
  Serial.println("\n\nDCF77 transmisor.\n");
  WiFi.disconnect();
  delay(1000);
  
tickerSetLow.attach_ms(100, DcfOut );

ledcSetup(0, 77500, 8);
ledcAttachPin(5, 0); // El pin 5 del ESP32 es el que usamos como antena. Con un cable de unos 50cm, colocado cerca del receptor, es más que suficiente.

  pinMode (ledPin, OUTPUT);
  digitalWrite (ledPin, LOW);
  
    WiFi.begin(ssid, password);

   int contador;
   while (WiFi.status() != WL_CONNECTED) {
        delay(200);
        if (contador > 10) ESP.restart();
        Serial.print ( "." );
    }

Serial.println("\n\nConexión WiFi completada.\n\n");
  
  configTzTime(TZ_INFO, NTP_SERVER);
  if (getLocalTime(&timeinfo, 10000)) {  // wait up to 10sec to sync
    Serial.println(&timeinfo, "Time set: %B %d %Y %H:%M:%S (%A)");
    epoch = time(&epoch); tv.tv_sec=time(&epoch);settimeofday(&tv,0);
                                   
  } else {
    Serial.println("Time not set");
  }
              
     //Serial.println ( epoch );  
     //Serial.println ( mktime(&timeinfo)) ;
     
     trabajo = epoch;
        
}


void loop() {


    CodificaTempo();
    
if (epoch > trabajo + 1200) {
 
        esp_sleep_enable_timer_wakeup( 3600 * 1000000);
        Serial.println("Parada de diez minutos ...");
        digitalWrite (ledPin, LOW);
        esp_deep_sleep_start();  }
}

   


void CodificaTempo() {

  int DayToEndOfMonth,DayOfWeekToEnd,DayOfWeekToSunday;

  dfc77 = time(&epoch) + 2*3600 + 60; // DCF77 envia el minuto siguiente al actual
 
   //calculate actual day to evaluate the summer/winter time of day ligh saving
    DayOfW = weekday(dfc77);
    Giorno = day(dfc77);
    Mese = month(dfc77);
    Anno = year(dfc77);
     /*
    Serial.print(">>> : ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print(Giorno);
    Serial.print('/');
    Serial.print(Mese);
    Serial.print('/');
    Serial.print(Anno);
    Serial.print(' ');
  */
    //calcolo ora solare o legale
    Dls = 0;    //default winter time
    //From April to september we are surely on summer time
    if (Mese > 3 && Mese < 10) {
      Dls = 1;
    };
    //March, month of change winter->summer time, last last sunday of the month
    //March has 31days so from 25 included on sunday we can be in summer time
    if (Mese == 3 && Giorno > 24) {
      DayToEndOfMonth = 31 - Giorno;
      DayOfWeekToSunday = 7 - DayOfW;
      if (DayOfWeekToSunday >= DayToEndOfMonth)
        Dls = 1;
    };
    //Octobee, month of change summer->winter time, l'ultima Domenica del mese
    //Even Octobee has 31days so from 25 included on sunday we can be in winter time
    if (Mese == 10) {
      Dls = 1;
      if (Giorno > 24) {
        DayToEndOfMonth = 31 - Giorno;
        DayOfWeekToEnd = 7 - DayOfW;
        if (DayOfWeekToEnd >= DayToEndOfMonth)
        Dls = 0;
      };
    };

    /*Serial.print("Dls:");
    Serial.print(Dls);
    Serial.print(' ');*/
    //add one hour if we are in summer time
    //if (Dls == 1)
    //  epoch += 3600;

    //now that we know the dls state, we can calculate the time too
    // print the hour, minute and second:
    Ore = hour(dfc77);
    Minuti = minute(dfc77);
    Secondi = second(dfc77);

/*    Serial.print(Ore); // print the hour
    Serial.print(':');
    Serial.print(Minuti); // print the minute
    Serial.print(':');
    Serial.println(Secondi); // print the second
*/

  int n,Tmp,TmpIn;
  int ParityCount = 0;

  //i primi 20 bits di ogni minuto li mettiamo a valore logico zero
  for (n=0;n<20;n++)
    ArrayImpulsi[n] = 1;

  //DayLightSaving bit
  if (Dls == 1)
    ArrayImpulsi[17] == 2;
  else
    ArrayImpulsi[18] == 2;
    
  //il bit 20 deve essere 1 per indicare tempo attivo
  ArrayImpulsi[20] = 2;

  //calcola i bits per il minuto
  TmpIn = Bin2Bcd(Minuti);
  for (n=21;n<28;n++) {
    Tmp = TmpIn & 1;
    ArrayImpulsi[n] = Tmp + 1;
    ParityCount += Tmp;
    TmpIn >>= 1;
  };
  if ((ParityCount & 1) == 0)
    ArrayImpulsi[28] = 1;
  else
    ArrayImpulsi[28] = 2;

  //calcola i bits per le ore
  ParityCount = 0;
  TmpIn = Bin2Bcd(Ore);
  for (n=29;n<35;n++) {
    Tmp = TmpIn & 1;
    ArrayImpulsi[n] = Tmp + 1;
    ParityCount += Tmp;
    TmpIn >>= 1;
  }
  if ((ParityCount & 1) == 0)
    ArrayImpulsi[35] = 1;
  else
    ArrayImpulsi[35] = 2;
   ParityCount = 0;
  //calcola i bits per il giorno
  TmpIn = Bin2Bcd(Giorno);
  for (n=36;n<42;n++) {
    Tmp = TmpIn & 1;
    ArrayImpulsi[n] = Tmp + 1;
    ParityCount += Tmp;
    TmpIn >>= 1;
  }
  //calcola i bits per il giorno della settimana  La libreria de Arduino el Domingo es 1 y el sabado 6, en DCF77  el domingo es 7 !!!
  DayOfW = DayOfW - 1; if (DayOfW == 0) DayOfW = 7;
  TmpIn = Bin2Bcd(DayOfW);
  for (n=42;n<45;n++) {
    Tmp = TmpIn & 1;
    ArrayImpulsi[n] = Tmp + 1;
    ParityCount += Tmp;
    TmpIn >>= 1;
  }
  //calcola i bits per il mese
  TmpIn = Bin2Bcd(Mese);
  for (n=45;n<50;n++) {
    Tmp = TmpIn & 1;
    ArrayImpulsi[n] = Tmp + 1;
    ParityCount += Tmp;
    TmpIn >>= 1;
  }
  //calcola i bits per l'anno
  TmpIn = Bin2Bcd(Anno - 2000);   //a noi interesa solo l'anno con ... il millenniumbug !
  for (n=50;n<58;n++) {
    Tmp = TmpIn & 1;
    ArrayImpulsi[n] = Tmp + 1;
    ParityCount += Tmp;
    TmpIn >>= 1;
  }
  //parita' di data
  if ((ParityCount & 1) == 0)
    ArrayImpulsi[58] = 1;
  else
    ArrayImpulsi[58] = 2;

  //ultimo impulso mancante
  ArrayImpulsi[59] = 0;

//for debug: print the whole 180 secs array       Serial.print(':'); for (n=0;n<60;n++){  Serial.print(ArrayImpulsi[n]); } Serial.println();

}

int Bin2Bcd(int dato) {
  int msb,lsb;
  if (dato < 10)
    return dato;
  msb = (dato / 10) << 4;
  lsb = dato % 10; 
  return msb + lsb;
}



 void DcfOut() {

  
  
    switch (ContaImpulsiParziale++) {
      case 0:
   
        if (ArrayImpulsi[Secondi] != 0) {
         digitalWrite(ledPin, LOW);  
         ledcWrite(0, 0); }
         break;
      case 1:
        if (ArrayImpulsi[Secondi] == 1) {
         digitalWrite(ledPin, HIGH); 
        ledcWrite(0, 127); }
        break;
      case 2:
        digitalWrite(ledPin, HIGH); 
        ledcWrite(0, 127);
        break;
      case 9:
                ContaImpulsiParziale = 0;
                

   if (Secondi == 1 || Secondi == 15 || Secondi == 21  || Secondi == 29 ) Serial.print("-");
   if (Secondi == 36  || Secondi == 42 || Secondi == 45  || Secondi == 50 ) Serial.print("-");

            if (ArrayImpulsi[Secondi] == 1) Serial.print("0");
            if (ArrayImpulsi[Secondi] == 2) Serial.print("1"); 
            
            if (Secondi == 59 ) {Serial.println(); } 
               
               
        break;
    };
    
  };
