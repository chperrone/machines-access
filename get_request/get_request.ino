#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// WiFi information
const char* WIFI_SSID = "Artisan's Asylum";
const char* WIFI_PSK = "I won't download stuff that will get us in legal trouble.";

// Remote site information
const char* http_site = "www.adafruit.com";
const char* ip = "172.16.10.77";
const int http_port = 80;

// Pin definitions
const int LED_PIN = 5;

// this is a placeholder switch that simulates a card swipe
bool swipe = true;

// Global variables
WiFiClient client;

void flash() {
  digitalWrite(0, HIGH);
  delay(1000);
  digitalWrite(0, LOW);
  delay(1000);
}

// Perform an HTTP GET request to a remote page
bool getPage() {

  delay(500);
  
  // Attempt to make a connection to the remote server
  if ( !client.connect(http_site, http_port) ) {
    Serial.println("Could connect to remote server");
    return false;
  }

  Serial.println("Making Get Request...");
  
  // We now create a URI for the request
  //String url = "/index.html";
  String url = "/testwifi/index.html";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + http_site + "\r\n" + 
               "Connection: close\r\n\r\n");
  
  return true;
}
// Attempt to connect to WiFi
void connectWiFi() {

  // Set WiFi mode to station (client)
  //WiFi.mode(WIFI_STA);
  
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
  Serial.print("GET Example");
  
  // Set up pin 0 for the relay switch
  pinMode(0, OUTPUT);
  
  connectWiFi();

  delay(1000);

  if (swipe) {
    // Attempt to connect to website
    if ( !getPage() ) {
      Serial.println("GET request failed");
    }

    Serial.println("the card is good!");

    Serial.println();
    flash();
    
    // Close socket and wait for disconnect from WiFi
    client.stop();
    if ( WiFi.status() != WL_DISCONNECTED ) {
      WiFi.disconnect();
    }
  }
  
}

void loop() {

  /*
  // If there are incoming bytes, print them
  if ( client.available() ) {
    char c = client.read();
    Serial.print(c);
  }
  
  // If the server has disconnected, stop the client and WiFi
  if ( !client.connected() ) {
    Serial.println();
    
    // Close socket and wait for disconnect from WiFi
    client.stop();
    if ( WiFi.status() != WL_DISCONNECTED ) {
      WiFi.disconnect();
    }
    
    // Do nothing
    Serial.println("Finished Thing GET test");
    while(true){
      delay(1000);
    }
    */
  //}
}
