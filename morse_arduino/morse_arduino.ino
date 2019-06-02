#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SevenSegmentTM1637.h>


#define highThr         200 // threshold between short and long input
#define pauseThr        300 // new letter after this time LOW on button
#define sig               0 // number of the button pin
#define clk               2 // pin definitions for TM1367
#define dio              16 // pin definitions for TM1367
#define abcLen           62 // length of the binary tree

const char *abc      =  "ETIANMSURWDKGOHVF\0L\0PJBXCYZQ\0\054S3\0\0D2\0\0+\0\0\0J16=/\0C\0H\07\0GN8\090";
const char *ssid     =  "Ru-Z 2.4Ghz";
const char *password =  "f0rdmustang";
const char *topicPub =  "morse/791548";
const char *topicSub =  "morse/791548";

// globals
uint8_t    currPos   =   1; // current position in binary tree "abc"
uint8_t    bttnState = LOW; // last state of button
uint8_t    s         =   0; // state of DFA
uint32_t   t         =   0; // time in ms
char       disp[5]   = "    "; // display data

SevenSegmentTM1637    display(clk, dio);
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

  display.begin();            // initializes the display
  display.setBacklight(100);  // set the brightness to 100 %
  display.print("INIT");      // display INIT on the display

  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {Serial.println("Not connected yet..."); delay(1000);}

  mac = WiFi.macAddress();
  uint16_t l = mac.length();
  char buf[l];
  mac.toCharArray(buf, l + 1);

  mqtt.setServer("test.mosquitto.org", 1883);
  mqtt.setCallback(onReceive);
  while (!mqtt.connect(buf)) delay(500);
  Serial.println("MQTT connected");
  mqtt.subscribe(topicSub);
  display.print("    ");
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
  if (47 < letter && 91 > letter) {
    Serial.print("I wrote: \"");
    Serial.print(letter);
    Serial.print("\"\n");
    String message = mac + "," + letter;
    uint16_t l = message.length();
    char buf[l];
    message.toCharArray(buf, l + 1);
    mqtt.publish(topicPub, buf);
  }
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
    display.print(disp);
    Serial.print("Changed display to:");
    Serial.println(disp);

  }
}

void appendChar(char c) {
 for (int i = 1; i < 4; i++) {
  if(disp[i] != NULL){
    disp[i - 1] = disp[i];
  }
 }
 disp[3] = c;
 disp[4] = '\0';
}
    
