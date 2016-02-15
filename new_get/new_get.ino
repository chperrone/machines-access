#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <MFRC522.h> // card reader library
#include <TextFinder.h>
#include <TimeLib.h>
#include <WiFiUDP.h>

#define RST_PIN         5
#define SS_PIN          4
#define SWITCH_PIN      0
#define Red             16
#define Green           15
#define Blue            2
#define BUTTONS         A0

//Wifi credentials
const char* WIFI_SSID = "Artisan's Asylum";
const char* WIFI_PSK = "I won't download stuff that will get us in legal trouble.";
// Remote site information
const char* ip = "172.16.11.34";
const int http_port = 8080;

const int array_size = 10;
String card_array[array_size];
String led_color;
const int timeZone = -5;  // Eastern Standard Time (USA)

IPAddress timeServer(132, 163, 4, 102); // time-a.timefreq.bldrdoc.gov

// class Instances
WiFiUDP Udp;
WiFiClient client;
TextFinder finder( client );
MFRC522 mfrc522(SS_PIN, RST_PIN);

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

/////////////////////////////////////////////

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmiting NTP Request...");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Received NTP Response!");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}


/////////////////////////////////////////////
///////// LED COLORS
/////////////////////////////////////////////

void setColor(int red, int green, int blue) { 
  digitalWrite(Red, red == 1 ? LOW : HIGH);
  digitalWrite(Green, green == 1 ? LOW : HIGH);
  digitalWrite(Blue, blue == 1 ? LOW : HIGH);  
}

void makeRed() {
  setColor(1, 0, 0);
}

void makeRed(int holdTime) {
  setColor(1, 0, 0);
  delay(holdTime);
}

void makeBlue() {
  setColor(0, 0, 1);
}

void makeBlue(int holdTime) {
  setColor(0, 0, 1);
  delay(holdTime);
}

void makeGreen() {
  setColor(0, 1, 0);
}

void makeGreen(int holdTime) {
  setColor(0, 1, 0);
  delay(holdTime);
}

void makeOrange() {
  setColor(1, 1, 0);
}

void makeOrange(int holdTime) {
  setColor(1, 1, 0);
  delay(holdTime);
}

void updateColor() {
  if (led_color == "blue") {
    makeBlue();
  } else if (led_color == "green") {
    makeGreen();
  } else if (led_color == "red") {
    makeRed();
  } else if (led_color == "orange") {
    makeOrange();
  }
}

//////////////////////////////////////////////////////
////////// DATA MGMT
//////////////////////////////////////////////////////

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
    line.trim();
    card_array[count] = line;
    count++;
  }
}

////////////////////////////////////////////////////////////
////////// NETWORKING 
////////////////////////////////////////////////////////////

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
               
  int timeout = millis() + 5000;
  while (client.available() == 0) {
    if (timeout - millis() < 0) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return false;
    }
  }

  Serial.println("Got a response");
  buildList(client);
  listToSerial(); //for debugging
  client.stop();
  return true;
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
    if (card_array[i].equals("")) { //were at the end of the list
      return false;
    }
    if (card_array[i].equals(card)) {
      return true;
    }
  }
  
  return false;
}

bool logUsage(String card) {
  if ( !client.connect(ip, http_port) ) {
    return false;
  }
  
  String url = "/log/" + card;
  // This will send the request to the server
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + ip + "\r\n" +
               "Connection: close\r\n\r\n");

}

//////////////////////////////////////////////////
////////// BUTTONS
//////////////////////////////////////////////////

void updateButtonHandler() {
  Serial.println("update ping");
  makeGreen();
  if ( !getFile() ) {
    Serial.println("Error: could not access server, could not update list");
  }
  delay(5000);
}

void maintenanceButtonHandler() {
  Serial.println("maintenance ping");
  delay(5000);
}


void globalButtonListener() {
  int voltage = analogRead(BUTTONS);

  if (voltage <= 920 && voltage >= 900) {
    updateButtonHandler();
  } if (voltage == 1024) {
    maintenanceButtonHandler();
  }
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
////////// ACCESS PROJECT PROTOTYPE
/////////////////////////////////////////////////////////

void setup() {
  
  // Set up serial console to read web page
  Serial.begin(115200);
  Serial.print("Prototype ");

  // Set up pin 0 for the relay switch
  pinMode(SWITCH_PIN, OUTPUT);
  pinMode(RST_PIN, LOW); //Tricky little bit, the pin gets pulled up by the card reader

  pinMode(Red, OUTPUT);
  pinMode(Green, OUTPUT);
  pinMode(Blue, OUTPUT);
  
  digitalWrite(Red, HIGH);
  digitalWrite(Green, HIGH);
  digitalWrite(Blue, HIGH); 

  connectToWifi();

  // initialize NTP clock
  Udp.begin(8888);
  setSyncProvider(getNtpTime);

  //grab the card list
  if ( !getFile() ) {
    Serial.println("GET Request failed, could not access card list");
    led_color = "red";
  }

  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522

  Serial.println();
  Serial.println("Finished Set Up");
}

void loop() {

  if (hour() == 23 && minute() == 59 && second() == 59) { //if its midnight, update the list
      if ( !getFile() ) {
        Serial.println("Nightly update failed");
      }
  }

  if (card_array[0].equals("")) { //if the list is not filled, something went wrong on server side
    led_color = "red";
  }
  else {
    led_color = "blue";
  }
  
  updateColor();
  globalButtonListener();
  
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
  listToSerial();
  Serial.println(card_array[0]);
  if (card.equals(card_array[0])) {
    Serial.println("yes");
  }

  // Attempt to 
  if ( ! canAccess(card) ) {
    Serial.println("MEMBER NOT AUTHORIZED: machine will NOT turn on.\n");
    makeRed(1000);
    return;
  }

  Serial.println("MEMBER AUTHORIZED: machine powering up.\n");
  makeGreen(1000);
  power();
  logUsage(card);
}
