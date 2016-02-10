#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <MFRC522.h> // card reader library
#include <TextFinder.h>

#define RST_PIN         5
#define SS_PIN          4
#define SWITCH_PIN      0

//Wifi credentials
const char* WIFI_SSID = "Artisan's Asylum";
const char* WIFI_PSK = "I won't download stuff that will get us in legal trouble.";

// Remote site information
const char* ip = "172.16.11.67";
const int http_port = 8080;

const int array_size = 500;
String card_array[array_size];

// class Instances
WiFiClient client;
TextFinder finder( client );

//RestClient client = RestClient(ip,http_port);
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

/*
 * Supply power to the machine. In other words,
 * send a logic signal from the designated pin.
 */
void power() {
  digitalWrite(SWITCH_PIN, HIGH);
  delay(1000);
  digitalWrite(SWITCH_PIN, LOW);
  delay(1000);
}

/*
 * Print the locally stored list of approved cards
 * to the Serial Monitor. Because the array is of
 * a finitely defined length, this function only 
 * goes so far as there are relevant values remaining
 */
void listToSerial() {
  Serial.println();
  Serial.println("Local List:");
  for (int i = 0; i < sizeof(card_array); i++) {
    if (!card_array[i].equals("")) { //continue?
      Serial.println(card_array[i]);
    } else {
      break;
    }
  }
}

// Takes a an incoming wifiClient and parses csv output 
void buildList(WiFiClient myClient) {
  Serial.println("Building local list from incoming client request");
  
  int count = 0;
  finder.find("\n\r"); //skip to the beginning of the csv output
  
  // Read all the lines of the reply from server and print them to Serial
  while(myClient.available()){
    if (count==(array_size - 1)) {
      Serial.println("Received too many card, numbers. The local array is not big enough");
      break;
    }
    
    String line = myClient.readStringUntil(',');
    card_array[count] = line;
    count++;
  }
}

// Perform an HTTP GET request for a static csv file
bool getFile() {
  // Attempt to make a connection to the remote server
  if ( !client.connect(ip, http_port) ) {
    return false;
  }
  
  Serial.println("Making Get Request...");
  
  // create a URL
  String url = "/woodshop.csv";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + ip + "\r\n" +
               "Connection: close\r\n\r\n");

  delay(50);
               
  int timeout = millis() + 5000;
  while (client.available() == 0) {
    if (timeout - millis() < 0) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return false;
    }
  }

  buildList(client);
  listToSerial(); //for debugging
  client.stop();
  return true;
}

String parseCard(MFRC522 mfrc522) {
  byte card_number[4];
  String result;

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    card_number[i] = mfrc522.uid.uidByte[i];
  }

  for (byte i = 0; i < sizeof(card_number); i++) {
    String x = String(card_number[i] < 0x10 ? "0" : "");
    String y = String(card_number[i], HEX);

    x.concat(y);
    result.concat(x);
  }

  return result;
}

void connectToWifi() {
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

/*
 * Searches the locally stored list of card numbers
 * for the given card.
 */
bool canAccess(String card) {
  for (int i=0; i < sizeof(card_array); i++) {
    if (!card_array[i].equals("")) {
      if (card_array[i].equals(card)) {
        return true;
      } 
      
    } else {
      return false;
    }
  }
  
  return false;
}

void setup() {

  // Set up serial console to read web page
  Serial.begin(115200);
  Serial.print("Prototype ");

  // Set up pin 0 for the relay switch
  pinMode(SWITCH_PIN, OUTPUT);
  pinMode(RST_PIN, LOW); //Tricky little bit, the pin gets pulled up by the card reader

  connectToWifi();
  delay(50);
  getFile();

  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522

  delay(1000);

  Serial.println();
  Serial.println("Finished Set Up");
  Serial.println();
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
  String card = parseCard(mfrc522);
  Serial.print(F("Card UID:"));
  Serial.println(card);

  // Attempt to connect to website
  if ( !canAccess(card) ) {
    Serial.println("Member not authorized: machine will not turn on");
    return;
  }

  Serial.println("Member authorized: machine powering up.");
  power();
}
