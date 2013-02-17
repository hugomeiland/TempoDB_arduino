/*
  tempo-DB arduino sensor client
 
 This sketch connects an analog sensor to tempo-DB (http://www.tempo-DB.com)
 using a Wiznet Ethernet shield. You can use the Arduino Ethernet shield, or
 the Adafruit Ethernet shield, either one will work, as long as it's got
 a Wiznet Ethernet module on board. Also the Arduino Ethernet uses this module.
 
 This example has been modified to use the tempo-DB.com API. 
 To make it work:
 - register an account on http://www.tempo-DB.com 
 - create a database
 - note the APIkey and APIsecret, and enter them below
 - you may want to change the series key
 
 make sure you have the base64 library installed from:
 https://github.com/adamvr/arduino-base64
 
 Circuit:
 * Analog sensor attached to analog in 0
 * Ethernet shield attached to pins 10, 11, 12, 13
 
 created 15 March 2010
 modified 9 Apr 2012
 by Tom Igoe with input from Usman Haque and Joe Saavedra
 modified to support TempoDB on 17 feb 2013
 by Hugo Meiland

 This code is in the public domain.
 */

#include <SPI.h>
#include <Ethernet.h>
#include <Base64.h>

#define DEBUG 0

// some settings to get this code to work (or just to tweak)
// format <APIkey>:<APIsecret> or actually just plain <username>:<password> since this is used
// for authentication and selection of the database 
char authPlain[] = "08a5906bbb664572bdf74ab254be5245:3363207897594111980d81d3305df6b6"; // <APIkey>:<APIsecret>
String seriesKey="arduino";
const unsigned long postingInterval = 10*1000; // 60 seconds delay between updates to tempo-DB.com
char server[] = "api.tempo-db.com";   // name address for tempo-DB API
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(108,168,208,188);      // numeric IP for api.tempo-db.com (be aware, this may change!)

// assign a MAC address for the ethernet controller.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
// fill in your address here:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  
// normally this sketch will try DHCP, but you may want to fill in an available IP address on your network here,
// for manual configuration:
IPAddress ip(10,0,1,20);
// initialize the library instance:
EthernetClient client;

// some other placeholders for variables in the code...
unsigned long lastConnectionTime = 0;          // last time you connected to the server, in milliseconds
boolean lastConnected = false;                 // state of the connection last time through the main loop
char authBase64[200]="";                       // will be used to hold the base64 encoded authentication key

// setup runs once during bootup, so use this to set up the environment...
void setup() {
  // base64 encode the authorization key
  base64_encode(authBase64, authPlain, sizeof(authPlain)-1); 
  
 // for debugging: open serial communications and wait for port to open:
  if (DEBUG) {
    Serial.begin(9600);
      while (!Serial) {
      ; // wait for serial port to connect. Needed for Leonardo only
      }
  }

  if (DEBUG) Serial.println("running setup...");

 // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    if (DEBUG) Serial.println("Failed to configure Ethernet using DHCP");
    // DHCP failed, so use a fixed IP address:
    Ethernet.begin(mac, ip);
  }
}

// the loop just keeps running till the end of all times...
void loop() {
  // with a TMP36GZ on the A0 pin...
  // read the analog sensor and do some magic to provide a "real" temperature.
  float sensorReading = ((analogRead(A0) * .004882814) - 0.5)*100;
  
  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  if (client.available()) {
    char c = client.read();
    if (DEBUG) Serial.print(c);
  }
  
  
  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    if (DEBUG) Serial.println("disconnecting.");
    client.stop();
  }

  // and this is where the magic happens:
  // if you're not connected, and the interval time have passed since
  // your last connection, then connect again and send data:
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    sendData(sensorReading);
  }
  // store the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
}

// this method makes a HTTP connection to the server:
void sendData(float sensorData) {
  // create the JSON formatted DataString using the sensorData value and calculate the length of the resulting
  String dataString = makeDataString(sensorData);
  // you might like to look at this when debugging:
  if (DEBUG) Serial.println(dataString);
  if (DEBUG) Serial.println(authBase64);

  // if there's a successful connection:
  if (client.connect(server, 80)) {
    if (DEBUG) Serial.println("connecting...");
    // send the HTTP PUT request:
    client.print("POST /v1/series/key/");
    client.print(seriesKey);
    client.println("/data/ HTTP/1.1");
    // add the authentication:
    client.print("Authorization: Basic ");
    client.println(authBase64);
    // just for the info on the server: show them you use this code:
    client.println("User-Agent: tempodb-arduino/0.1");
    // just for info of a schizofrene server
    client.println("Host: api.tempo-db.com");
    // string to provide the content-length in the HTTP header.
    client.print("Content-Length: ");
    client.println(dataString.length());
    // last pieces of the HTTP PUT request:
    client.println("Content-Type: application/x-www-form-urlencoded");
    // a blank line is required here before sending the actual data:
    client.println();
    // here's the actual content of the POST request:
    client.println(dataString);
  } 
  else {
    // if you couldn't make a connection:
    if (DEBUG) {
      Serial.println("connection failed");
      Serial.println("disconnecting.");
    }
    client.stop();
  }
   // note the time that the connection was made or attempted:
  lastConnectionTime = millis();
}

// make a (short) JSON string to upload the sensor value
String makeDataString(long sensorData) {
  String returnString = String("[{\"v\":");  
  returnString += String(sensorData, DEC); 
  returnString += String("}]");  
return returnString; 
}


