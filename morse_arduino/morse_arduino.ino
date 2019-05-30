#include <ESP8266WiFi.h>
#include <ESP8266MQTTClient.h>
#include <Arduino_JSON.h>


#define highThr       200 // threshold between short and long input
#define pauseThr      300 // new letter after this time LOW on button
#define buttonPin       0 // the number of the button pin
#define abcLen         62 // length of the abc array

// consts
const char *abc      =  "ETIANMSURWDKGOHVF\0L\0PJBXCYZQ\0\054S3\0\0D2\0\0+\0\0\0J16=/\0C\0H\07\0GN8\090";
const char *ssid     =  "Landownunder";
const char *password =  "icomefroma";
const char *topic    =  "messages";

// globals
uint8_t currPos      =   1; // current position in binary tree "abc"
uint8_t buttnState   = LOW; // last state of button
uint8_t s            =   0; // state of DFA
uint32_t t           =   0; // time in ms

JSONVar msg;


// declarations
void setup();
void loop();
void sendLetter(char letter);
void sendLetter(uint8_t pos);
char getLetter(uint8_t pos);


// implementations
void setup() {
  Serial.begin(115200);
  // while ( !Serial ) delay(10);
  pinMode(buttonPin, INPUT);
  Serial.print("\nConnecting to network ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(100);

  msg["sender"] = "STBY4";

  //client.begin("mqtt://test.mosquitto.org/");
}

void loop() {
  int b = digitalRead(buttonPin);


  if (LOW == buttnState) {
    if (HIGH == b) {
      // button has been unpressed, is pressed now
      if (2 * currPos > abcLen) {
        sendLetter(currPos);
        currPos = 1;
      }
      t = millis();
    } else {
      // button still unpressed
      uint32_t dT = millis() - t;
      if (1 < currPos && dT > pauseThr) {
        sendLetter(currPos);
        currPos = 1;
      }
    }
  }
  else if (HIGH == buttnState && LOW == b) {
    // button has been pressed, is unpressed now
    uint32_t dT = millis() - t;
    uint8_t newPos = currPos * 2;
    if (dT > highThr) ++newPos;
    if (newPos < abcLen) currPos = newPos;
    t = millis();
  }

  buttnState = b;
}

void sendLetter(char letter) {
  // TODO publish letter
  Serial.println(letter);

  msg["char"] = letter;
  String json = JSON.stringify(msg);
  Serial.println(json);

  // client.publish(topic, json);
}

void sendLetter(uint8_t pos) {
  sendLetter(getLetter(pos));
}

char getLetter(uint8_t pos) {
  return abc[pos - 2];
}
