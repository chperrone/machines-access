#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         5
#define SS_PIN          4
#define SWITCH_PIN      0       

//Wifi credentials
const char* WIFI_SSID = "Artisan's Asylum";
const char* WIFI_PSK = "I won't download stuff that will get us in legal trouble.";

// Remote site information
const char* ip = "172.16.11.27";
const int http_port = 8080;



// class Instances
WiFiClient client;
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void flash() {
  digitalWrite(SWITCH_PIN, HIGH);
  delay(1000);
  digitalWrite(SWITCH_PIN, LOW);
  delay(1000);
}


// Helper routine to dump a byte array as hex values to Serial
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

// Perform an HTTP GET request to a remote page
bool getPage() {
  delay(500);
  
  // Attempt to make a connection to the remote server
  if ( !client.connect(ip, http_port) ) {
    return false;
  }

  Serial.println();
  Serial.println("Making Get Request...");
  
  // We now create a URI for the request
  String url = "/api";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + ip + "\r\n" + 
               "Connection: close\r\n\r\n");

  if (client.find(true)) {
    return true;
  } 
  else { return false; }
}
// Attempt to connect to WiFi
void connectWiFi() {

  Serial.println("Attempting to connect to WiFi");
  
  // Initiate connection with SSID and PSK
  WiFi.begin(WIFI_SSID, WIFI_PSK);
  
  // Blink LED while we wait for WiFi connection
  while ( WiFi.status() != WL_CONNECTED ) {
    Serial.println(".");
    delay(500);
  }
  
  Serial.println("WiFi Connected!!");
}

void setup() {
  
  // Set up serial console to read web page
  Serial.begin(115200);
  Serial.print("Prototype");
  
  // Set up pin 0 for the relay switch
  //pinMode(0, OUTPUT);
  pinMode(RST_PIN, LOW); //Tricky little bit, the pin gets pulled up by the card reader

  connectWiFi();

  delay(500);
  
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  
  delay(1000);
  
}

void loop() {
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  // Show some details of the PICC (that is: the tag/card)
  Serial.print(F("Card UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  
  // Attempt to connect to website
  if ( !getPage() ) {
    Serial.println("GET request failed");
  }
  
  flash();
    
  // Close socket
  client.stop();
}
