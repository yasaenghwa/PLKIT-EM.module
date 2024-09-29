#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define TdsSensorPin 5  // TDS 센서 핀(GPIO 34)
#define VREF 3.3         // ESP32 Vref
#define SCOUNT  30       // 샘플 카운트
int analogBuffer[SCOUNT];  // ADC 값 버퍼
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;

const char* ssid = "PLKit";
const char* password = "987654321";
const char* mqtt_server = "ec2-52-79-219-88.ap-northeast-2.compute.amazonaws.com";

WiFiClient espClient;
PubSubClient client(espClient);

const long mqttInterval = 10000; 
unsigned long previousMillis = 0;

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

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("MQTT connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setKeepAlive(120);

  pinMode(TdsSensorPin, INPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) analogBufferIndex = 0;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= mqttInterval) {
    previousMillis = currentMillis;

    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * VREF / 4095.0;
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;
    tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage 
              - 255.86 * compensationVoltage * compensationVoltage 
              + 857.39 * compensationVoltage) * 0.5;

    // JSON 객체 생성
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["tds"] = tdsValue; // TDS 값 추가

    // JSON 데이터를 문자열로 변환
    char jsonBuffer[200];
    serializeJson(jsonDoc, jsonBuffer);

    // MQTT로 JSON 메시지 전송
    Serial.print("Publishing JSON message: ");
    Serial.println(jsonBuffer);

    if (client.publish("PLKIT/sensor/TDS", jsonBuffer, true)) {
      Serial.println("JSON message published successfully");
    } else {
      Serial.println("Failed to publish JSON message");
    }
  }
  
  delay(100);  // 100ms 대기
}

// 중간값 필터링 함수
int getMedianNum(int bArray[], int iFilterLen) {
  // 입력된 배열을 복사하여 중간 값을 구할 배열 생성
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];

  int i, j, bTemp;
  
  // 버블 정렬을 사용하여 배열을 오름차순으로 정렬
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }

  // 배열 길이가 홀수인 경우, 중간값을 반환
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  // 배열 길이가 짝수인 경우, 중간 두 값의 평균을 반환
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  
  return bTemp;  // 최종 중간값 반환
}
