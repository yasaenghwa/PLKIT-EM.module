#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "network_ip_info.h"  // 보안 데이터 처리 헤더 파일

// 릴레이 제어 핀 설정
const int ledRelayPin = 6;  // LED 릴레이 핀 (GPIO 6)
const int fanRelayPin = 17;  // Fan 릴레이 핀 (GPIO 17)

// MQTT 클라이언트 설정
WiFiClient espClient;
PubSubClient client(espClient);

// Wi-Fi 연결 함수
void setup_wifi() {
  delay(10);
  Serial.println();
  const char* ssid = decryptData(getEncryptedSSID());
  const char* password = decryptData(getEncryptedPassword());
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// MQTT 재연결 함수 (QoS 설정 포함)
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      // 각 제어 토픽 구독
      client.subscribe("PLKIT/control/Light", 1);  // LED 제어 토픽
      client.subscribe("PLKIT/control/fan", 1);  // Fan 제어 토픽
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);  // 연결 실패 시 2초 후 재시도
    }
  }
}

// LED 제어 함수
void controlLED(String command) {
  if (command == "on") {
    digitalWrite(ledRelayPin, HIGH);  // 릴레이 활성화, LED 켜짐
    Serial.println("LED turned ON");
  } else if (command == "off") {
    digitalWrite(ledRelayPin, LOW);   // 릴레이 비활성화, LED 꺼짐
    Serial.println("LED turned OFF");
  }
}

// Fan 제어 함수
void controlFan(String command) {
  if (command == "on") {
    digitalWrite(fanRelayPin, HIGH);  // 릴레이 활성화, Fan 켜짐
    Serial.println("Fan turned ON");
  } else if (command == "off") {
    digitalWrite(fanRelayPin, LOW);   // 릴레이 비활성화, Fan 꺼짐
    Serial.println("Fan turned OFF");
  }
}

// MQTT 메시지 수신 콜백 함수 (JSON 파싱 포함)
void callback(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';  // 문자열 종료

  // JSON 파싱
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (!error) {
    String command = doc["command"].as<String>();

    // 해당 토픽에 따라 제어
    if (String(topic) == "PLKIT/control/Light") {
      controlLED(command);  // LED 제어
    } else if (String(topic) == "PLKIT/control/fan") {
      controlFan(command);  // Fan 제어
    }
  } else {
    Serial.println("Failed to parse JSON");
  }
}

void setup() {
  Serial.begin(115200);  // 시리얼 통신 초기화
  pinMode(ledRelayPin, OUTPUT);
  digitalWrite(ledRelayPin, LOW);  // LED 초기 상태는 OFF

  pinMode(fanRelayPin, OUTPUT);
  digitalWrite(fanRelayPin, LOW);  // Fan 초기 상태는 OFF

  setup_wifi();  // Wi-Fi 연결 설정
  const char* mqtt_server = decryptData(getEncryptedMqttServer());
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);  // MQTT 콜백 함수 설정
}

void loop() {
  if (!client.connected()) {
    reconnect();  // MQTT 서버에 재연결 시도
  }
  client.loop();  // MQTT 메시지 수신 및 처리
}
