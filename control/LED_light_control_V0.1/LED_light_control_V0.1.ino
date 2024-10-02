#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Wi-Fi 정보
const char* ssid = "PLKit";
const char* password = "987654321";

// MQTT 서버 정보
const char* mqtt_server = "ec2-52-79-219-88.ap-northeast-2.compute.amazonaws.com";

// 릴레이 제어 핀 설정 (GPIO 17번 사용)
const int relayPin = 17;

// MQTT 클라이언트 설정
WiFiClient espClient;
PubSubClient client(espClient);

// Wi-Fi 연결 함수
void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
}

// MQTT 재연결 함수 (QoS 설정 포함)
void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      client.subscribe("test", 1);  // QoS 1로 "test" 토픽 구독
    } else {
      delay(2000);  // 연결 실패 시 2초 후 재시도
    }
  }
}

// LED 제어 함수
void controlLED(String command) {
  if (command == "1") {
    digitalWrite(relayPin, HIGH);  // 릴레이 활성화, LED 켜짐
  } else if (command == "0") {
    digitalWrite(relayPin, LOW);   // 릴레이 비활성화, LED 꺼짐
  }
}

// MQTT 메시지 수신 콜백 함수 (JSON 파싱 포함)
void callback(char* topic, byte* payload, unsigned int length) {
  // 메시지 버퍼에 저장
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
    controlLED(command);  // LED 제어
  }
}

void setup() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);  // 초기 상태는 OFF

  setup_wifi();  // Wi-Fi 연결 설정
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);  // MQTT 콜백 함수 설정
}

void loop() {
  if (!client.connected()) {
    reconnect();  // MQTT 서버에 재연결 시도
  }
  client.loop();  // MQTT 메시지 수신 및 처리
}
