#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <TM1637Display.h>


#define highThr         200 // threshold between short and long input
#define pauseThr        300 // new letter after this time LOW on button
#define sig               2 // number of the button pin
#define clk               0 // pin definitions for TM1367
#define dio              15 // pin definitions for TM1367
#define abcLen           62 // length of the binary tree


// constants
const uint8_t seg[]  = {
  SEG_A | SEG_F | SEG_B | SEG_E | SEG_C | SEG_D, // 0
  SEG_B | SEG_C,                                 // 1
  SEG_A | SEG_B | SEG_G | SEG_E | SEG_C,         // 2
  SEG_A | SEG_B | SEG_G | SEG_C | SEG_D,         // 3
  SEG_F | SEG_B | SEG_G | SEG_C,                 // 4
  SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,         // 5
  SEG_A | SEG_F | SEG_G | SEG_E | SEG_C | SEG_D, // 6
  SEG_A | SEG_B | SEG_C,                         // 7
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // 8
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G, // 9
  SEG_G | SEG_D,                                 // :
  SEG_G | SEG_D,                                 // ;
  SEG_G | SEG_D,                                 // <
  SEG_G | SEG_D,                                 // =
  SEG_G | SEG_D,                                 // >
  SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,         // ?
  SEG_G | SEG_E | SEG_D | SEG_C,                 // @
  SEG_A | SEG_F | SEG_B | SEG_G | SEG_E | SEG_C, // A
  SEG_F | SEG_G | SEG_E | SEG_C | SEG_D,         // b
  SEG_A | SEG_F | SEG_E | SEG_D,                 // C
  SEG_B | SEG_G | SEG_E | SEG_C | SEG_D,         // d
  SEG_A | SEG_F | SEG_G | SEG_E | SEG_D,         // E
  SEG_A | SEG_F | SEG_G | SEG_E,                 // F
  SEG_A | SEG_F | SEG_B | SEG_G | SEG_C, SEG_D,  // g
  SEG_F | SEG_B | SEG_G | SEG_E | SEG_C,         // H
  SEG_B | SEG_C,                                 // I
  SEG_B | SEG_C | SEG_D,                         // J
  SEG_F | SEG_G | SEG_E | SEG_D,                 // k
  SEG_F | SEG_E | SEG_D,                         // L
  SEG_G | SEG_E | SEG_C,                         // m
  SEG_G | SEG_E | SEG_C,                         // n
  SEG_G | SEG_E | SEG_C | SEG_D,                 // o
  SEG_A | SEG_F | SEG_B | SEG_G | SEG_E,         // P
  SEG_A | SEG_F | SEG_B | SEG_G | SEG_C,         // q
  SEG_A | SEG_F | SEG_B | SEG_G | SEG_D,         // R
  SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,         // S
  SEG_A | SEG_F | SEG_E,                         // T
  SEG_F | SEG_E | SEG_D | SEG_C | SEG_B,         // U
  SEG_F | SEG_E | SEG_D | SEG_C | SEG_B,         // V
  SEG_F | SEG_E | SEG_D | SEG_C | SEG_B | SEG_G, // W
  SEG_F | SEG_B | SEG_G | SEG_E | SEG_C,         // X
  SEG_F | SEG_G | SEG_B | SEG_C,                 // Y
  SEG_A | SEG_B | SEG_G | SEG_E | SEG_C,         // Z
};
const char *abc      =  "ETIANMSURWDKGOHVF\0L\0PJBXCYZQ\0\054S3\0\0D2\0\0+\0\0\0J16=/\0C\0H\07\0GN8\090";
const char *ssid     =  "Landownunder";
const char *password =  "icomefroma";
const char *topicPub =  "morse/791548";
const char *topicSub =  "morse/791548";

// globals
uint8_t    currPos   =   1; // current position in binary tree "abc"
uint8_t    bttnState = LOW; // last state of button
uint8_t    s         =   0; // state of DFA
uint32_t   t         =   0; // time in ms
uint8_t    disp[]    = {0x00, 0x00, 0x0, 0x00}; // display data

TM1637Display tm1637(clk, dio);
WiFiClient client;
PubSubClient mqtt(client);
String       mac;           // MAC addr. is used as uid


// declarations
void setup();
void loop();
void sendLetter(char letter);
void sendLetter(uint8_t pos);
char getLetter(uint8_t pos);
void onReceive(char* topic, byte* payload, unsigned int length);
void appendChar(char c);


// implementations
void setup() {
  Serial.begin(115200);
  while ( !Serial ) delay(10);
  pinMode(sig, INPUT);
  pinMode(clk, OUTPUT);
  pinMode(dio, OUTPUT);

  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(100);

  mac = WiFi.macAddress();
  uint16_t l = mac.length();
  char buf[l];
  mac.toCharArray(buf, l + 1);

  mqtt.setServer("test.mosquitto.org", 1883);
  mqtt.setCallback(onReceive);
  while (!mqtt.connect(buf)) delay(500);
  Serial.println("MQTT connected");
  mqtt.subscribe(topicSub);

  tm1637.setBrightness(0x0f);
}

void loop() {
  mqtt.loop();
  int b = digitalRead(sig);

  if (LOW == bttnState) {
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
  else if (HIGH == bttnState && LOW == b) {
    // button has been pressed, is unpressed now
    uint32_t dT = millis() - t;
    uint8_t newPos = currPos * 2;
    if (dT > highThr) ++newPos;
    if (newPos < abcLen) currPos = newPos;
    t = millis();
  }

  bttnState = b;
}

void sendLetter(char letter) {
  Serial.print("I wrote: \"");
  Serial.print(letter);
  Serial.print("\"\n");
  String message = mac + "," + letter;
  uint16_t l = message.length();
  char buf[l];
  message.toCharArray(buf, l + 1);
  mqtt.publish(topicPub, buf);
}

void sendLetter(uint8_t pos) {
  sendLetter(getLetter(pos));
}

char getLetter(uint8_t pos) {
  return abc[pos - 2];
}

void onReceive(char* topic, byte* payload, unsigned int length) {
  char sender[length];
  char letter;
  uint8_t part = 0;
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    if (',' == c) {
      part = 1;
      letter = (char)payload[i + 1];
      sender[i] = '\0';
    }
    if (0 == part) {
      sender[i] = c;
    }
  }

  uint16_t l = mac.length();
  char buf[l];
  mac.toCharArray(buf, l + 1);

  if (0 != strcmp(sender, buf)) {
    // sender is someone else
    Serial.print(sender);
    Serial.print(" wrote: \"");
    Serial.print(letter);
    Serial.print("\"\n");

    // output to display
    appendChar(letter);
    tm1637.setSegments(disp);
  }
}

void appendChar(char c) {
  for (int i = 1; i < 4; ++i) {
    disp[i - 1] = disp[i];
  }
  if (47 < c && 91 > c) {
    disp[3] = seg[c - 48];
  } else {
    disp[3] = 0x00;
  }
}
