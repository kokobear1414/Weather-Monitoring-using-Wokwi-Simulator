#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <FastLED.h>

// Defining Pins
#define DHTPIN 12
#define LED 26
#define SERVO_PIN 2
#define LED_PIN 4
#define NUM_LEDS 16

// LDR Pin
#define LDR_PIN 15   // 4-pin LDR module AO → D15

// DHT parameters
#define DHTTYPE DHT22
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

// Servo motor
Servo servo;

// WS2812 LED strip
CRGB leds[NUM_LEDS];

// WiFi + MQTT
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "broker.hivemq.com";
const char* clientID = "ujaisldaalkjlasdfgh;laslksdja1";
const char* topic = "Tempdata";

unsigned long previousMillis = 0;
const long interval = 1000;
String msgStr = "";
float temp, hum;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(clientID)) {
      Serial.println("MQTT connected");
      client.subscribe("lights");
      client.subscribe("servo");
      client.subscribe("lights/neopixel");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" retry in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String data = "";
  for (int i = 0; i < length; i++) {
    data += (char)payload[i];
  }

  if (String(topic) == "lights") {
    digitalWrite(LED, data == "ON" ? HIGH : LOW);
  }
  else if (String(topic) == "servo") {
    servo.write(data.toInt());
  }
  else if (String(topic) == "lights/neopixel") {
    int r,g,b;
    sscanf(data.c_str(), "%d,%d,%d", &r,&g,&b);
    fill_solid(leds, NUM_LEDS, CRGB(r,g,b));
    FastLED.show();
  }
}

void setup() {
  Serial.begin(115200);

  dht.begin();
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  pinMode(LDR_PIN, INPUT);   // <-- LDR pin

  servo.attach(SERVO_PIN);
  servo.write(0);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);

  setup_wifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    sensors_event_t event;
    dht.temperature().getEvent(&event);

    if (!isnan(event.temperature)) temp = event.temperature;

    dht.humidity().getEvent(&event);
    if (!isnan(event.relative_humidity)) hum = event.relative_humidity;

    // -----------------------
    // Read LDR
    // -----------------------
    int ldrValue = analogRead(LDR_PIN);
    Serial.print("LDR Value: ");
    Serial.println(ldrValue);

    // Publish data (temp, hum, LDR)
    msgStr = String(temp) + "," + String(hum) + "," + String(ldrValue) + ",";
    char msg[msgStr.length() + 1];
    msgStr.toCharArray(msg, msgStr.length() + 1);

    Serial.print("PUBLISH DATA: ");
    Serial.println(msgStr);

    client.publish(topic, msg);
    msgStr = "";
  }
}
