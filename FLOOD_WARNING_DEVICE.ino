
#define DEBUG 0
//#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
// #include <ezBuzzer.h>
#include "DHT.h"

const String PHONE_NUMBER = "";
char ssid[] = "";
char pass[] = "";

// #define LEVEL_1 60
// #define LEVEL_2 55
// #define LEVEL_3 53

#define LEVEL_1 50
#define LEVEL_2 40
#define LEVEL_3 30

#define ECHO_PIN 18
#define TRIG_PIN 5

#define RX_PIN 4
#define TX_PIN 2

#define BUZZER_PIN 19
#define DHT_PIN 27
#define DHT_TYPE DHT11
#define BAUD_RATE 9600

long duration;
int distance;

float temperature;
float humidity;

int currentLevel;

HardwareSerial hwSerial(1);
// ezBuzzer buzzer(BUZZER_PIN);
DHT dht(DHT_PIN, DHT_TYPE);

BlynkTimer calcDistance;
BlynkTimer getDHTData;
BlynkTimer wifiReconnect;
BlynkTimer sendData;

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(DHT_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Initializing...");

  hwSerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  dht.begin();

  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN);

  // NON BLOCKING TASK
  calcDistance.setInterval(1000L, calcDistanceCb);
  wifiReconnect.setInterval(1000L, wifiReconnectCb);
  sendData.setInterval(1000L, sendDataCb);
  getDHTData.setInterval(1000L, getDHTDataCb);

  delay(1000);

  hwSerial.println("AT");
  updateSerial();
  hwSerial.println("AT+CSQ");
  updateSerial();
  hwSerial.println("AT+CCID");
  updateSerial();
  hwSerial.println("AT+CREG?");
  updateSerial();
}

void loop() {
  //buzzer.loop();
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
    sendData.run();
  }
  calcDistance.run();
  getDHTData.run();
  wifiReconnect.run();

  if (currentLevel == 1) {
    for (int i = 0; i < 3; i++) {
      tone(BUZZER_PIN, 1000);
      delay(500);
      noTone(BUZZER_PIN);
      delay(500);
    }
    delay(500);
  } else if (currentLevel == 2) {
    for (int i = 0; i < 5; i++) {
      tone(BUZZER_PIN, 1000);
      delay(250);
      noTone(BUZZER_PIN);
      delay(250);
    }
    delay(250);
  } else if (currentLevel == 3) {
    for (int i = 0; i < 10; i++) {
      tone(BUZZER_PIN, 1000);
      delay(100);
      noTone(BUZZER_PIN);
      delay(100);
    }
    delay(100);
  }
}

void updateSerial() {
  delay(500);
  while (Serial.available()) {
    hwSerial.write(Serial.read());
  }
  while (hwSerial.available()) {
    Serial.write(hwSerial.read());
  }
}

void sendWarningMessage(String level) {
  hwSerial.println("AT+CMGF=1");
  updateSerial();
  hwSerial.println("AT+CMGS=\"" + PHONE_NUMBER + "\"");
  updateSerial();
  // SMS MESSAGE
  hwSerial.print("Flood Alert!" + level);
  updateSerial();
  Serial.println();
  Serial.println("Message Sent");
  hwSerial.write(26);
}

void makeCall() {
  hwSerial.println("AT");
  updateSerial();
  hwSerial.println("ATD+ " + PHONE_NUMBER + ";");
  updateSerial();
  // delay(20000);
  // hwSerial.println("ATH");
  // updateSerial();
}

void calcDistanceCb() {
  Serial.println(WiFi.status());
  Serial.println(Blynk.connected());
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.0344 / 2;

#if DEBUG
  Serial.println("Debug mode is enabled.");
#else
  if (distance < LEVEL_1 && distance > LEVEL_2) {
    currentLevel = 1;
    //buzzer.beep(1000);
    Serial.println("Level 1 Warning");
    sendWarningMessage("LEVEL 1");
  } else if (distance < LEVEL_2 && distance > LEVEL_3) {
    currentLevel = 2;
    Serial.println("Level 2 Warning");
    sendWarningMessage("LEVEL 2");
  } else if (distance < LEVEL_3) {
    currentLevel = 3;
    Serial.println("Level 3 Warning");
    makeCall();
    delay(1000);
    sendWarningMessage("LEVEL 3");
  } else {
    currentLevel = 0;
  }
#endif

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
}

void getDHTDataCb() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  Serial.println(temperature);
  Serial.println(humidity);
}

void wifiReconnectCb() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Attempting to reconnect Wi-Fi...");
    WiFi.disconnect();
    WiFi.begin(ssid, pass);
  }
}

void sendDataCb() {
  Blynk.virtualWrite(V0, distance);
  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V2, humidity);
  Blynk.virtualWrite(V3, currentLevel);
}
