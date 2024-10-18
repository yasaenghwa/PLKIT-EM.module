#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include "network_ip_info.h"

// 릴레이 제어 핀 설정
const int ledRelayPin = 6;  // LED 릴레이 핀 (GPIO 6)
const int fanRelayPin = 17;  // Fan 릴레이 핀 (GPIO 17)

// BLE 설정
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789012"
#define SSID_CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654321"
#define PASSWORD_CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654322"

static BLEAddress *pServerAddress = nullptr;
bool doConnect = false;
bool connected = false;
BLEClient* pClient = nullptr;
String ssid = "";      
String password = "";  

// WiFi 및 MQTT 클라이언트 설정
WiFiClient espClient;
PubSubClient client(espClient);

// Wi-Fi 연결 함수
void connectToWiFi(const char* ssid, const char* password) {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected successfully!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// MQTT 재연결 함수
void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
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

// BLE 클라이언트 콜백 클래스 (onConnect, onDisconnect 함수 구현)
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) override {
    Serial.println("Connected to BLE server.");
  }

  void onDisconnect(BLEClient* pclient) override {
    Serial.println("Disconnected from BLE server.");
  }
};

// 서버에 연결하여 두 값을 수신 (BLE)
void connectToServer() {
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());  // 수정된 콜백 설정

  // 서버에 연결 시도
  pClient->connect(*pServerAddress);
  Serial.println("Connecting to BLE server...");

  // 서비스와 특성 검색
  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find BLE service UUID");
    return;
  }

  // SSID 특성 읽기
  BLERemoteCharacteristic* pSSIDCharacteristic = pRemoteService->getCharacteristic(SSID_CHARACTERISTIC_UUID);
  if (pSSIDCharacteristic == nullptr) {
    Serial.println("Failed to find SSID characteristic UUID");
    return;
  }
  ssid = pSSIDCharacteristic->readValue().c_str();
  Serial.println("Received SSID: " + ssid);

  // Password 특성 읽기
  BLERemoteCharacteristic* pPasswordCharacteristic = pRemoteService->getCharacteristic(PASSWORD_CHARACTERISTIC_UUID);
  if (pPasswordCharacteristic == nullptr) {
    Serial.println("Failed to find Password characteristic UUID");
    return;
  }
  password = pPasswordCharacteristic->readValue().c_str();
  Serial.println("Received Password: " + password);

  // 값 수신 후 명시적으로 연결 끊기
  pClient->disconnect();
}

// BLE 광고된 장치 콜백 클래스
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // 서비스 UUID가 매칭되는지 확인
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))) {
      Serial.println("Found a BLE server with matching UUID.");
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      advertisedDevice.getScan()->stop();  // 서버를 찾으면 스캔 중지
      doConnect = true;  // 연결 플래그 설정
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE scan...");

  BLEDevice::init("");

  // 릴레이 핀 설정
  pinMode(ledRelayPin, OUTPUT);
  digitalWrite(ledRelayPin, LOW);  // LED 초기 상태는 OFF

  pinMode(fanRelayPin, OUTPUT);
  digitalWrite(fanRelayPin, LOW);  // Fan 초기 상태는 OFF

  // BLE 스캔 설정 및 시작
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);  // 30초 동안 스캔
}

void loop() {
  // BLE 서버를 찾았고 연결할 준비가 되었으면 연결 시도
  if (doConnect) {
    connectToServer();
    doConnect = false;

    // Wi-Fi 연결 시도
    if (ssid != "" && password != "") {
      connectToWiFi(ssid.c_str(), password.c_str());
      client.setServer(mqtt_server, 1883);
      client.setCallback(callback);  // MQTT 콜백 함수 설정
    }
  }

  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    reconnect();  // MQTT 서버에 재연결 시도
  }

  client.loop();  // MQTT 메시지 수신 및 처리
}
