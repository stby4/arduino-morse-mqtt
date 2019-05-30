#include <ESP8266WiFi.h>
#include <PubSubClient.h>


#define highThr         200 // threshold between short and long input
#define pauseThr        300 // new letter after this time LOW on button
#define buttonPin         0 // number of the button pin
#define abcLen           62 // length of the binary tree


// constants
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


// implementations
void setup() {
  Serial.begin(115200);
  // while ( !Serial ) delay(10);
  pinMode(buttonPin, INPUT);
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
}

void loop() {
  mqtt.loop();
  int b = digitalRead(buttonPin);

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

    // OUTPUT TO DISPLAY HERE
  }
}
