#include <ESP8266WiFi.h>
#include <PubSubClient.h>



#define highThr       200 // threshold between short and long input
#define pauseThr      300 // new letter after this time LOW on button
#define buttonPin       0 // the number of the button pin
#define abcLen         62 // length of the abc array

// consts
const char *abc      =  "ETIANMSURWDKGOHVF\0L\0PJBXCYZQ\0\054S3\0\0D2\0\0+\0\0\0J16=/\0C\0H\07\0GN8\090";
const char *ssid     =  "Landownunder";
const char *password =  "icomefroma";
const char *topicPub =  "channels/791548/publish/EV5C1YH9KFJ88CKQ";
const char *topicSub =  "channels/791548/subscribe/csv/MS83GOAH49SF5S0L";

const char *mqttUser = "stby04";
const char *mqttPass = "6VDSH6HM6NAZMZIQ";

// globals
uint8_t currPos      =   1; // current position in binary tree "abc"
uint8_t bttnState   = LOW; // last state of button
uint8_t s            =   0; // state of DFA
uint32_t t           =   0; // time in ms

WiFiClient client;
PubSubClient mqtt(client);
String mac;


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
  Serial.print("\nConnecting to network ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(100);

  mac = WiFi.macAddress();
  //mac[sMac.length()+1];
  //sMac.toCharArray(mac, sMac.length());

  mqtt.setServer("mqtt.thingspeak.com", 1883);
  mqtt.setCallback(onReceive);
  while(!mqtt.connect("1234", mqttUser, mqttPass)) delay(500);
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
  Serial.println(letter);
  String message = "field1=" + mac + "&field2=" + letter;
  uint16_t l = message.length();
  char buf[l];
  message.toCharArray(buf,l+1);
  //mqtt.publish(topicPub, buf);
  mqtt.publish(topicPub, "field1=42");
}

void sendLetter(uint8_t pos) {
  sendLetter(getLetter(pos));
}

char getLetter(uint8_t pos) {
  return abc[pos - 2];
}

void onReceive(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
