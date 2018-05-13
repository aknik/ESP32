/*
  LoRa Duplex communication based on Tom Igoe example of Arduino LoRa library
  customized for TTGO LoRa32 V2.0 Oled Board

  Sends a message every 2 or 3 second, and get incoming messages. 
  Implements a one-byte addressing scheme, with broadcast address.

  Note: while sending, LoRa radio is not listening for incoming messages.
*/
#include <SPI.h>              // include libraries
#include <LoRa.h>    //https://github.com/sandeepmistry/arduino-LoRa
#include "SSD1306.h" //https://github.com/ThingPulse/esp8266-oled-ssd1306

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


//////////////////////CONFIG 1///////////////////////////
byte localAddress = 8;     // address of this device
byte destination = 18;     // destination to send to
int interval = 3000;       // interval between sends
String message = "Penny.";    // send a message
///////////////////////////////////////////////////////


/*
//////////////////////CONFIG 2///////////////////////////
byte localAddress = 18;    // address of this device
byte destination = 8;      // destination to send to
int interval = 2000;       // interval between sends
String message = "Hello!"; // send a message
///////////////////////////////////////////////////////
*/

String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
long lastSendTime = 0;        // last send time

SSD1306 display(OLED_ADDR, OLED_SDA, OLED_SCL);

void printScreen() {
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.setColor(BLACK);
  display.fillRect(0, 0, 127, 30);
  display.display();

  display.setColor(WHITE);
  display.drawString(0, 00, "LoRa Duplex sender " + String(localAddress));
  display.drawString(0, 10, "Me: " + String(localAddress)
                          + "  To: " + String(destination)
                          + " N: " + String(msgCount));
  display.drawString(0, 20, "Tx PKT: " + message);
  display.display();
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  printScreen();
  
  Serial.println("Sending message " + String(msgCount) + " to address: "+ String(destination));
  Serial.println("Message: " + message); 
  Serial.println();
  delay(1000);
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
  Serial.println("error");
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

  display.setColor(BLACK);
  display.fillRect(0, 32, 127, 61);
  display.display();

  display.setColor(WHITE);
  display.drawLine(0,31,127,31);
  display.drawString(0, 32, "Rx PKT: " + incoming);

  display.drawString(0, 42, "RSSI: " + String(LoRa.packetRssi())
                          + " SNR: " + String(LoRa.packetSnr()));
  display.drawString(0, 52, "FR:"  + String(sender)
                          + " TO:" + String(recipient)
                          + " LN:" + String(incomingLength)
                          + " ID:" + String(incomingMsgId));
  display.display();

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

  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.clear();

  Serial.begin(115200);                   // initialize serial
  while (!Serial);
  Serial.println("TTGO LoRa32 V2.0 P2P");
  display.drawString(0, 00, "TTGO LoRa32 V2.0 P2P");
  display.display();

  SPI.begin(SX1278_SCK, SX1278_MISO, SX1278_MOSI, SX1278_CS);
  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(SX1278_CS, SX1278_RST, SX1278_DI0);// set CS, reset, IRQ pin

  if (!LoRa.begin(LORA_BAND))
  {             // initialize ratio at 868 MHz
    Serial.println("LoRa init failed. Check your connections.");
    display.drawString(0, 10, "LoRa init failed");
    display.drawString(0, 20, "Check connections");
    display.display();
    while (true);                       // if failed, do nothing
  }

  //LoRa.onReceive(onReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");
  display.drawString(0, 10, "LoRa init succeeded.");
  display.display();
  delay(1500);
  display.clear();
  display.display();
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

