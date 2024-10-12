#include <WiFi.h>           
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "network_ip_info.h"  // 보안 데이터 처리 헤더 파일

// WiFi 및 MQTT 클라이언트 초기화
WiFiClient espClient;
PubSubClient client(espClient);

// CDS 조도 센서 설정
#define CDS_SENSOR_PIN 7  // CDS 조도 센서 연결 핀 (GPIO 5)

// 메시지 전송 간격 설정 (10초)
const long interval = 10000; 
unsigned long previousMillis = 0;

void setup_wifi() 
{
  delay(10);
  Serial.println();
  const char* ssid = decryptData(getEncryptedSSID());
  const char* password = decryptData(getEncryptedPassword());
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attemptCounter = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");

    if (attemptCounter % 5 == 0) {
      Serial.println();
      Serial.println("Wi-Fi 연결 중...");
    }

    attemptCounter++;
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() 
{
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {  
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200); 
  setup_wifi();
  const char* mqtt_server = decryptData(getEncryptedMqttServer());
  client.setServer(mqtt_server, 1883);
  client.setKeepAlive(120); 
  
  pinMode(CDS_SENSOR_PIN, INPUT); // CDS 센서 핀 설정
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // CDS 조도 센서 읽기 (0-4095 범위의 값)
    int lightLevel = analogRead(CDS_SENSOR_PIN);

    // 조도 값을 퍼센트로 변환 (예시, 조정 필요할 수 있음)
    int lightPercent = map(lightLevel, 0, 4095, 0, 100);

    // 데이터 출력
    Serial.print("Light Level: ");
    Serial.print(lightLevel);
    Serial.print(" (");
    Serial.print(lightPercent);
    Serial.println("%)");

    // JSON 객체 생성
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["light_level"] = lightPercent;

    char jsonBuffer[200];
    serializeJson(jsonDoc, jsonBuffer);

    Serial.print("Publishing JSON message: ");
    Serial.println(jsonBuffer);

    if (client.publish("PLKIT/sensor/Light/01", jsonBuffer, true)) 
    {
      Serial.println("JSON message published successfully");
    } 
    else 
    {
      Serial.println("Failed to publish JSON message");
    }
  }
}
