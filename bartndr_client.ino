/* Bartndr System Firmware
 *
 * Emily Chen and Maruchi Kim
 */

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include "unoclient.h"
#include <JsonParser.h>
#include <Adafruit_NeoPixel.h>

using namespace ArduinoJson::Parser;

#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "HTC One_ 0715"           // cannot be longer than 32 characters!
//#define WLAN_SSID       "CalVisitor"           // cannot be longer than 32 characters!

#define WLAN_PASS       "67F513AD"
//#define WLAN_PASS       ""

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2
//#define WLAN_SECURITY   WLAN_SEC_UNSEC


#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.


int myPumps[] = {A1, A2, A3, A4, A5};
int bottles[5][5] = {{10000, 10000, 10000, 10000, 10000}, 
                     {0, 25000, 0, 25000, 0}, 
                     {15000, 0, 15000, 0, 15000}, 
                     {0, 25000, 0, 0, 25000}, 
                     {25000, 0, 25000, 0, 0}};
                     
//int bottles[5][5] = {{1, 1, 1, 1, 1}, 
//                     {1, 1, 1, 1, 1}, 
//                     {1, 1, 1, 1, 1}, 
//                     {1, 1, 1, 1, 1}, 
//                     {1, 1, 1, 1, 1}};



Adafruit_NeoPixel strip = Adafruit_NeoPixel(90, 2, NEO_GRB + NEO_KHZ800);
RestClient client = RestClient("api.bartndr.me");
JsonParser<5> parser;
String taskID;
int drinkID = -1;
short SIZE_OF_JSON = 50;
int lastFsrVal = 0;
int statusCode;
String response;

void setup_wifi() {
 for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 255));
 }
 strip.show();
 Serial.println(F("Hello, CC3000!\n")); 
  
  /* Initialise the module */
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  // Optional SSID scan
  // listSSIDResults();
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
  
  Serial.println(F("Connected!"));
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 255));
  }
  strip.show();
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0));
  }
  strip.show();
}


//Setup
void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.show();
  setup_wifi();
  Serial.println("setup completed");
}

void getDrink() {
  char json[SIZE_OF_JSON];
  response = "";
  statusCode = client.get("/task/new", &response);
  response.toCharArray(json, SIZE_OF_JSON);
  Serial.println("GET");
  JsonObject root = parser.parse(json);
  taskID = root["task_id"];
  char* drinkChar = root["item_name"];
  drinkID = atoi(drinkChar); 
  drinkID -= 1; 
  Serial.println(drinkID);
  Serial.println(taskID);
}

void postDrink() {
  char postRoute[35];
  String("/task/complete?task_id=" + taskID).toCharArray(postRoute, 35);
  Serial.println("posting");
  statusCode = client.post(postRoute, NULL, &response);
  Serial.println("test");
}

int averageAnalogRead(int pinToRead) {
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 

  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return(runningValue);  
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int cupPlacement() {
  int maxCup = 400;
  int emptyCup = 620;
  int fsrVal = averageAnalogRead(A0);
  if ((lastFsrVal - fsrVal ) > 5) { // indicate cup is present
    Serial.println("Start Mixing!");
    lastFsrVal = fsrVal;
    return 1;
  }
  else if (fsrVal > emptyCup) {
    Serial.println("Ready for New Cup");
    lastFsrVal = fsrVal;
    return 0;
  }
  lastFsrVal = fsrVal;
   return 0;
}

void startMixing() {
  // Find Max Val
  int maxVal = 0;

  for (int i = 0 ; i < 5 ; i++) {
    if (bottles[drinkID][i] > maxVal) {
      maxVal = bottles[drinkID][i];
    }
  }
  
  // Begin Mixing!
  for (int j = maxVal ; j > 0 ; j--) {
    if (bottles[drinkID][0] == j) {
      analogWrite(myPumps[0], 255);
    }
    if (bottles[drinkID][1] == j) {
      analogWrite(myPumps[1], 255);
    }
    if (bottles[drinkID][2] == j) {
      analogWrite(myPumps[2], 255);
    }
    if (bottles[drinkID][3] == j) {
      analogWrite(myPumps[3], 255);
    } 
    if (bottles[drinkID][4] == j) {
      analogWrite(myPumps[4], 255);
    }
    
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(j % 256, j % 256, j % 256));
    }
    strip.show();
  }
  analogWrite(myPumps[0], 0);
  analogWrite(myPumps[1], 0);
  analogWrite(myPumps[2], 0);
  analogWrite(myPumps[3], 0);
  analogWrite(myPumps[4], 0);
  
  //finished animation code here!!
  if (drinkID == 0) {
    theaterChase(strip.Color(245, 223, 61), 50); // White
  } else if (drinkID == 1) {
    theaterChase(strip.Color(230, 88, 158), 50); // Red
  } else if (drinkID == 2) {
    theaterChase(strip.Color(105, 199, 0), 50); // Green
  } else if (drinkID == 3) {
    theaterChase(strip.Color(58, 199, 219), 50); // Cyan  
  } else if (drinkID == 4) {
    theaterChase(strip.Color(172, 95, 184), 50); // Purple  
  }
  
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
//  delay(2000);
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0 ; j < 20 ; j++) {  //do 10 cycles of chasing
    for (int q = 0 ; q < 3 ; q++) {
      for (int i = 0 ; i < strip.numPixels() ; i = i + 3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i = 0 ; i < strip.numPixels() ; i = i + 3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void loop(){
  getDrink();
  if (drinkID < 0) {
    rainbowCycle(1);
  } 
  else {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(255, 100, 0));
    }
    strip.show();
    cupPlacement();
    cupPlacement();
    cupPlacement();
    cupPlacement();
    cupPlacement();
    cupPlacement();
    cupPlacement();
    cupPlacement();
    cupPlacement();
    cupPlacement();
    cupPlacement();
    cupPlacement();
    while (!cupPlacement());
    startMixing();
    postDrink();
  }
}


