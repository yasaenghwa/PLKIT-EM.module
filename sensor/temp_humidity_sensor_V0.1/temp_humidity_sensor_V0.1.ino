#include <WiFi.h>           // For ESP32, use the WiFi library
#include <PubSubClient.h>
#include <ArduinoJson.h>  // ArduinoJson 라이브러리 포함

// WiFi 정보
const char* ssid = "PLKit";  // Wi-Fi 이름
const char* password = "987654321";  // Wi-Fi 비밀번호

// MQTT 브로커 정보
const char* mqtt_server = "ec2-52-79-219-88.ap-northeast-2.compute.amazonaws.com"; // MQTT 브로커 주소

// WiFi 및 MQTT 클라이언트 초기화
WiFiClient espClient;
PubSubClient client(espClient);

// Soil Moisture Sensor 설정
#define SOIL_MOISTURE_PIN 34  // Soil moisture sensor 연결 핀 (GPIO 34)

// 메시지 전송 간격 설정 (10초)
const long interval = 10000; // 전송 주기 (밀리초)
unsigned long previousMillis = 0; // 이전 전송 시간 기록

// WiFi 연결 설정 함수
void setup_wifi() 
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attemptCounter = 0; // 연결 시도 횟수를 기록하기 위한 변수

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");  // 연결 시도 표시

    // 5초마다 "Wi-Fi 연결 중" 메시지 출력
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

// MQTT 재연결 함수
void reconnect() 
{
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {  // Use a unique client ID for ESP32
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
  Serial.begin(115200);  // Baud rate 설정
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setKeepAlive(120); // Keep-Alive 설정을 120초로 증가
  
  pinMode(SOIL_MOISTURE_PIN, INPUT); // Soil Moisture sensor 핀을 입력으로 설정
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();
  
  // 메시지 전송 주기 체크
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Soil moisture 센서 읽기 (0-4095 범위의 값)
    int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);

    // Soil moisture 값을 퍼센트로 변환 (예시, 보정이 필요할 수 있음)
    int soilMoisturePercent = map(soilMoistureValue, 0, 4095, 0, 100);

    // 데이터 출력
    Serial.print("Soil Moisture Value: ");
    Serial.print(soilMoistureValue);
    Serial.print(" (");
    Serial.print(soilMoisturePercent);
    Serial.println("%)");

    // JSON 객체 생성
    StaticJsonDocument<200> jsonDoc; // JSON 문서 생성
    jsonDoc["soil_moisture"] = soilMoisturePercent; // Soil moisture 데이터 추가

    char jsonBuffer[200];
    serializeJson(jsonDoc, jsonBuffer); // JSON 객체를 문자열로 변환

    Serial.print("Publishing JSON message: ");
    Serial.println(jsonBuffer);

    // JSON 메시지 전송
    if (client.publish("PLKIT/sensor/flow_rate/01", jsonBuffer, true)) 
    {
      Serial.println("JSON message published successfully");
    } 
    else 
    {
      Serial.println("Failed to publish JSON message");
    }
  }
}
