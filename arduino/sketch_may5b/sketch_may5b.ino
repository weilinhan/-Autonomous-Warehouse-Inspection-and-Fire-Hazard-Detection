#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "env.h"

// WiFi credentials
const char WIFI_SSID[] = "iPhone";
const char WIFI_PASSWORD[] = "13579246";

// AWS IoT parameters
const char THINGNAME[] = "ESP8266";
const char MQTT_HOST[] = "a1dtwlsw1olgol-ats.iot.eu-central-1.amazonaws.com";
const char AWS_IOT_PUBLISH_TOPIC[] = "esp8266/pub";
const char AWS_IOT_SUBSCRIBE_TOPIC[] = "esp8266/sub";

// Timezone
const int8_t TIME_ZONE = -5;

// Interval for publishing
const long interval = 5000;
unsigned long lastMillis = 0;

// Secure WiFi connection
WiFiClientSecure net;
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
PubSubClient client(net);

// DHT11 config
#define DHTPIN 4        // GPIO4 (D2)
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// NTP setup
void NTPConnect() {
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 1510592825) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
}

// Message reception callback
void messageReceived(char *topic, byte *payload, unsigned int length) {
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Connect to AWS IoT
void connectAWS() {
  delay(3000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  NTPConnect();

  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);

  client.setServer(MQTT_HOST, 8883);
  client.setCallback(messageReceived);

  Serial.println("Connecting to AWS IoT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(1000);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected!");
}

// Publish sensor data
void publishMessage() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int mq135_value = analogRead(A0);

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%  Temperature: ");
  Serial.print(temperature);
  Serial.print("°C  MQ135: ");
  Serial.println(mq135_value);

  StaticJsonDocument<256> doc;
  doc["time"] = millis();
  doc["humidity"] = humidity;
  doc["temperature"] = temperature;
  doc["mq135"] = mq135_value;

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void setup() {
  Serial.begin(115200);
  dht.begin();  // 初始化 DHT11
  connectAWS(); // 连接 AWS
}

void loop() {
  if (millis() - lastMillis > interval) {
    lastMillis = millis();
    if (client.connected()) {
      publishMessage();
    }
  }
  client.loop();
}
