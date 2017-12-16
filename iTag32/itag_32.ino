#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  20        /* Time ESP32 will go to sleep (in seconds) */


static void wifi_power_save(void);

BLECharacteristic *pCharacteristic1;
BLECharacteristic *pCharacteristic2;

const int ledPin = 2;
bool deviceConnected = false;
uint8_t value = 0;

#define S1_UUID    "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHAR1_UUID "0000ffe1-0000-1000-8000-00805f9b34fb"

#define S2_UUID    "00001802-0000-1000-8000-00805f9b34fb"
#define CHAR2_UUID "00002a06-0000-1000-8000-00805f9b34fb"

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic2) {
      
       std::string rxValue = pCharacteristic2->getValue();
       
 if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++){
          Serial.print(rxValue[i],HEX);
          if (rxValue[i] != 0) digitalWrite (ledPin, HIGH); 
          if (rxValue[i] == 0) digitalWrite (ledPin, LOW); }
        Serial.println();
        Serial.println("*********");
      }
    }
};


class MyServerCallbacks: public BLEServerCallbacks {
  
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected ...");
      
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected ......");
      Serial.println("ESP32 go to sleep for " + String(TIME_TO_SLEEP) + " s");
      esp_deep_sleep_start();
    }
    };

void setup() {
  Serial.begin(115200);
  pinMode (ledPin, OUTPUT);
  digitalWrite (ledPin, LOW);

esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
 
 uint8_t new_mac[] = {0xFF,0xFF,0xFF,0x08,0x08,0x06};
 //srand(esp_random());for (int i = 0; i < sizeof(new_mac); i++) { new_mac[i] = rand() % 256;}
 esp_base_mac_addr_set(new_mac);

  // Create the BLE Device
  BLEDevice::init("iTag32");

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service   // Create a BLE Characteristic
  BLEService *pService1 = pServer->createService(S1_UUID);
  pCharacteristic1 = pService1->createCharacteristic(
                      CHAR1_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
                    
  // Create the BLE Service   // Create a BLE Characteristic
  BLEService *pService2 = pServer->createService(S2_UUID);
  pCharacteristic2 = pService2->createCharacteristic(
                      CHAR2_UUID,
                      BLECharacteristic::PROPERTY_WRITE 
                                          );
                                          
  pCharacteristic2->setCallbacks(new MyCallbacks());                  
   
  // Create a BLE Descriptor
  pCharacteristic1->addDescriptor(new BLE2902());
  //pCharacteristic2->addDescriptor(new BLE2902());
  
  //pCharacteristic2->setValue("Hello World says Neil");

  // Start the service
  pService1->start();
  pService2->start();
  // Start advertising

  pServer->getAdvertising()->start();

  delay(5000);  
  
if (!deviceConnected) {      
      Serial.println("ESP32 go to sleep for " + String(TIME_TO_SLEEP) + " s");
      esp_deep_sleep_start();
    }

digitalWrite(ledPin, HIGH);delay(2000);digitalWrite (ledPin, LOW);
    
}



void loop() {

    
    if(!digitalRead(0) ){
      value = 1;
      Serial.printf("*** NOTIFY: %d ***\n", value);
      pCharacteristic1->setValue(&value, 1);
      pCharacteristic1->notify();
      
      while (!digitalRead(0)) {delay(20);} } 
      else {value = 0;pCharacteristic1->setValue(&value, 1);delay(100); }





    
        
    }
    
  

