#include <ESP8266WiFi.h>
#include <ESP8266MQTTClient.h>

#define highThr       500 // threshold between short and long input
#define pauseThr      500 // new letter after 500 ms LOW on button
#define buttonPin       2 // the number of the button pin
#define abcLen         62 // length of the abc array


struct packet
{
  String sender;
  char letter;
};
typedef struct packet Packet;

const char *abc      = "ETIANMSURWDKGOHVFÜLÄPJBXCYZQÖÖ54S3É\0D2\0È+\0\0ÀJ16=/\0C\0H\07\0GN8\090";
const char *ssid     = "Landownunder";
const char *password = "icomefroma";
const char *topic    = "messages";

uint8_t currPos   =     0; // current position in binary tree "abc"
uint8_t lastState =   LOW; // last state of button
uint32_t t        =     0; // time in ms

MQTTClient client;

void setup() {
  Serial.begin(115200);
  // while ( !Serial ) delay(10);
  pinMode(buttonPin, INPUT);
  Serial.print("\nConnecting to network ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(100);
  
  client.begin("mqtt://test.mosquitto.org/");
}

void loop() {
  delay(100);
  int b = digitalRead(buttonPin);

  if (LOW == lastState) {
    if(HIGH == b) {
      // button has been unpressed, is pressed now
      if(0 != currPos) {
        sendLetter(currPos);
        currPos = 0;
      }
      t = millis();
    } else {
      // button still unpressed
      uint32_t dT = t - millis();
      if(0 != currPos && dT > pauseThr) {
        sendLetter(currPos);
        currPos = 0;
      }
    }
  }
  else if (HIGH == lastState && LOW == b) {
    // button has been pressed, is unpressed now
    uint32_t dT = t - millis();
    uint8_t newPos = ((currPos + 2) * 2) - 2;
    if(dT > highThr) ++currPos;
    if(newPos < abcLen/2) currPos = newPos;
  }

  lastState = b;
}

void sendLetter(uint8_t pos) {
  // TODO publish letter
  char letter = abc[pos];
  Serial.println(letter);
  // client.publish(topic, letter);
}
