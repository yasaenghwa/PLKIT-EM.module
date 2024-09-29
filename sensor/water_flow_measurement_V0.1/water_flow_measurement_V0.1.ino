#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi 정보
const char* ssid = "PLKit";
const char* password = "987654321";

// MQTT 브로커 정보
const char* mqtt_server = "ec2-52-79-219-88.ap-northeast-2.compute.amazonaws.com";

// WiFi 및 MQTT 클라이언트 초기화
WiFiClient espClient;
PubSubClient client(espClient);

// 유량 측정 센서 설정
#define FLOWSENSORPIN 2
volatile uint16_t pulses = 0;
volatile float flowrate = 0.0;
hw_timer_t *timer = NULL;

// 메시지 전송 간격 (10초)
const long mqttInterval = 10000; 
unsigned long previousMillis = 0;

// 인터럽트 핸들러 함수 (펄스 수 증가)
void IRAM_ATTR handleInterrupt() {
  pulses++;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// MQTT 재연결 함수
void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setKeepAlive(120);

  pinMode(FLOWSENSORPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOWSENSORPIN), handleInterrupt, RISING);  // 유량 센서 인터럽트 설정

  // 타이머 설정 (1MHz 주파수 설정)
  timer = timerBegin(1000000); 
  timerAttachInterrupt(timer, []() { 
    flowrate = (pulses / 7.5) / 60.0;  // 유량 계산
    pulses = 0;  // 펄스 초기화
  });
  timerWrite(timer, 1000000);  // 1초마다 실행
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();
  
  // 10초마다 MQTT 메시지 전송
  if (currentMillis - previousMillis >= mqttInterval) {
    previousMillis = currentMillis;

    // JSON 객체 생성 및 데이터 추가
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["frequency"] = flowrate;
    jsonDoc["pulse"] = pulses;
    jsonDoc["liter"] = (flowrate / 60.0);

    // JSON 데이터를 문자열로 변환
    char jsonBuffer[200];
    serializeJson(jsonDoc, jsonBuffer);

    // MQTT로 JSON 메시지 전송
    Serial.print("Publishing JSON message: ");
    Serial.println(jsonBuffer);

    if (client.publish("PLKIT/sensor/Flow_Rate/01", jsonBuffer, true)) {
      Serial.println("JSON message published successfully");
    } else {
      Serial.println("Failed to publish JSON message");
    }
  }

  delay(100);  // 100ms 대기
}
