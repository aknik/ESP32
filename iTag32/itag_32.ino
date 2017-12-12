#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLECharacteristic *pCharacteristic;
BLECharacteristic *pCharacteristic2;

bool deviceConnected = false;
uint8_t value = 1;

#define SERVICE_UUID        "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000ffe1-0000-1000-8000-00805f9b34fb"

#define SERVICE2_UUID        "00001802-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC2_UUID "00002a06-0000-1000-8000-00805f9b34fb"


class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic2) {
      
       std::string rxValue = pCharacteristic2->getValue();
       
 if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i],HEX);

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
    }
    };

void setup() {
  Serial.begin(115200);


 uint8_t new_mac[] = {0xFF,0xFF,0xFF,0x08,0x08,0x06};
 //srand(esp_random());for (int i = 0; i < sizeof(new_mac); i++) { new_mac[i] = rand() % 256;}
 esp_base_mac_addr_set(new_mac);

  // Create the BLE Device
  BLEDevice::init("iTag32");

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service   // Create a BLE Characteristic
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
                    
  // Create the BLE Service   // Create a BLE Characteristic
  BLEService *pService2 = pServer->createService(SERVICE2_UUID);
  pCharacteristic2 = pService2->createCharacteristic(
                      CHARACTERISTIC2_UUID,
                      BLECharacteristic::PROPERTY_WRITE 
                                          );
                    
  pCharacteristic2->setCallbacks(new MyCallbacks());                  
   
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());
  //pCharacteristic2->addDescriptor(new BLE2902());
  
  pCharacteristic2->setValue("Hello World says Neil");
  
  // Start the service
  pService->start();
  pService2->start();
  // Start advertising
  pServer->getAdvertising()->start();

}

void loop() {
    
    if(!digitalRead(0) ){
      Serial.printf("*** NOTIFY: %d ***\n", value);
      pCharacteristic->setValue(&value, 1);
      pCharacteristic->notify();
      delay(100); }
    while (!digitalRead(0)) {delay(0);};
        
    }
    
  


