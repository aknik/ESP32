/*
  LoRa Duplex communication based on Tom Igoe example of Arduino LoRa library
  customized for TTGO LoRa32 V2.0 Oled Board

  Sends a message every 2 or 3 second, and get incoming messages. 
  Implements a one-byte addressing scheme, with broadcast address.

  Note: while sending, LoRa radio is not listening for incoming messages.
*/
#include <SPI.h>              // include libraries
#include <LoRa.h>    //https://github.com/sandeepmistry/arduino-LoRa

//Pinout! Customized for TTGO LoRa32 V2.0 Oled Board!
#define SX1278_SCK  5    // GPIO5  -- SX1278's SCK
#define SX1278_MISO 19   // GPIO19 -- SX1278's MISO
#define SX1278_MOSI 27   // GPIO27 -- SX1278's MOSI
#define SX1278_CS   18   // GPIO18 -- SX1278's CS
#define SX1278_RST  14   // GPIO14 -- SX1278's RESET
#define SX1278_DI0  26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

#define OLED_SDA    21    // GPIO21  -- OLED'S SDA
#define OLED_SCL    22    // GPIO22  -- OLED's SCL Shared with onboard LED! :(
#define OLED_RST    16    // GPIO16  -- OLED's VCC?

#define LORA_BAND   433E6 // LoRa Band (Europe)
#define OLED_ADDR   0x3c  // OLED's ADDRESS

/*
//////////////////////CONFIG 1///////////////////////////
byte localAddress = 8;     // address of this device
byte destination = 18;     // destination to send to
int interval = 3000;       // interval between sends
String message = "Hi!";    // send a message
///////////////////////////////////////////////////////
*/


//////////////////////CONFIG 2///////////////////////////
byte localAddress = 18;    // address of this device
byte destination = 8;      // destination to send to
int interval = 2000;       // interval between sends
String message = "Verchy"; // send a message
///////////////////////////////////////////////////////


String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
long lastSendTime = 0;        // last send time


void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  
  Serial.println("Sending message " + String(msgCount) + " to address: "+ String(destination));
  Serial.println("Message: " + message); 
  Serial.println();
  delay(1000);
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
  
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";                 // payload of packet

  while (LoRa.available()) {            // can't use readString() in callback, so
    incoming += (char)LoRa.read();      // add bytes one by one
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    incoming = "message length error";
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    incoming = "message is not for me";
    return;                             // skip rest of function
  }


  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
  delay(1000);
}

void setup() {
  pinMode(OLED_RST,OUTPUT);
  digitalWrite(OLED_RST, LOW);  // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(OLED_RST, HIGH); // while OLED running, must set GPIO16 in high
  delay(1000);


  Serial.begin(115200);                   // initialize serial
  while (!Serial);
  Serial.println("TTGO LoRa32 V2.0 P2P");

  SPI.begin(SX1278_SCK, SX1278_MISO, SX1278_MOSI, SX1278_CS);
  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(SX1278_CS, SX1278_RST, SX1278_DI0);// set CS, reset, IRQ pin

  if (!LoRa.begin(LORA_BAND))
  {             // initialize ratio at 868 MHz
    Serial.println("LoRa init failed. Check your connections.");

    while (true);                       // if failed, do nothing
  }

  //LoRa.onReceive(onReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");
  delay(1500);

}

void loop() {
  if (millis() - lastSendTime > interval) {
    sendMessage(message);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;     // 2-3 seconds
    LoRa.receive();                     // go back into receive mode
  }
  int packetSize = LoRa.parsePacket();
  if (packetSize) { onReceive(packetSize);  }
}

