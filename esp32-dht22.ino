/*
   ESP32 + DHT22 + Ultrasonic Sensor + LED + Buzzer + HTTP POST
*/

#include "DHTesp.h"
#include "WiFi.h"
#include "HTTPClient.h"
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

DHTesp dhtSensor;
const int trigPin = 5;
const int echoPin = 18;
const int DHT_PIN = 15;
const int RED = 13;
const int YELLOW = 12;
const int GREEN = 14;
const int BUZZER_PIN = 27;
int limit = 400;
long duration;
float distanceCm;
const char* url = "localhost:8000/api/alert"; 

void setup() {
  Serial.begin(9600);
  WiFi.disconnect();
  WiFi.begin("Wokwi-GUEST", "", 6);
  while ((!(WiFi.status() == WL_CONNECTED))) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

void get_dht_sensor(float &temperature, float &humidity) {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  temperature = data.temperature;
  humidity = data.humidity;
  Serial.println("Temp: " + String(temperature, 2) + "°C");
  Serial.println("Humidity: " + String(humidity, 1) + "%");
}

void get_distance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * SOUND_SPEED / 2;
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
}

void send_alert(float temperature, float humidity, float waterLevel) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String payload = "{";
    payload += "\"kelembapan\": \"" + String(humidity) + "\",";
    payload += "\"suhu\": \"" + String(temperature) + "\",";
    payload += "\"ketinggianAir\": \"" + String(waterLevel) + "\"}";

    int httpResponseCode = http.POST(payload);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void check_flood_level(float temperature, float humidity) {
  if (distanceCm > limit) {  
    digitalWrite(RED, HIGH);
    digitalWrite(YELLOW, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    tone(BUZZER_PIN, 1000);
    send_alert(temperature, humidity, distanceCm);

  } else if (distanceCm <= limit && distanceCm >= (2 * limit / 3)) {  
   digitalWrite(RED, LOW);
    digitalWrite(YELLOW, HIGH);
    digitalWrite(GREEN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    noTone(BUZZER_PIN);

  } else if (distanceCm < (2 * limit / 3) && distanceCm >= 0) {  
   digitalWrite(RED, LOW);
    digitalWrite(YELLOW, LOW);
    digitalWrite(GREEN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
    noTone(BUZZER_PIN);
  }
}


void loop() {
  float temperature, humidity;
  get_dht_sensor(temperature, humidity);
  get_distance();
  check_flood_level(temperature, humidity);
  Serial.println("---");
  delay(500);
}