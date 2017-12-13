// Receptor o cliente de un iTag o su emulador con ESP32. Solo necesitamos la direccion MAC del pulsador (itag)

#include "BLEDevice.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("0000ffe0-0000-1000-8000-00805f9b34fb");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("0000ffe1-0000-1000-8000-00805f9b34fb");

static BLEAddress *pServerAddress;
static BLERemoteCharacteristic* pRemoteCharacteristic;

const int ledPin = 2;

void setup() {
   Serial.begin(115200);
   Serial.println("Starting Arduino BLE Client application...");
   BLEDevice::init("");
   delay(500);
   
   // DIRECCION MAC DEL SERVIDOR ITAG AL QUE NOS CONECTAMOS
   
   pServerAddress = new BLEAddress("FF:FF:FF:08:08:08");
   delay(500);
   Serial.print("Forming a connection to ");
   Serial.println(pServerAddress->toString().c_str());
   BLEClient* pClient = BLEDevice::createClient();
   Serial.println("Created client");
   pClient->connect(*pServerAddress);
   Serial.println("Connected to server");
   delay(500);
     // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
          }else{
    Serial.println(" - Found our service");}

// Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());

    }
    Serial.println(" - Found our characteristic");

    
}
void loop() {
  
    // Read the value of the characteristic.
    uint8_t  rxValue = pRemoteCharacteristic->readUInt8();;

     if (rxValue > 0) { 
      Serial.println(rxValue);
      delay (300);
      pRemoteCharacteristic->writeValue(0);
     }
     
   
  
delay(0);
  
  } // The End
  
  
  
  
