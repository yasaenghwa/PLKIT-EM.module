#include <WiFi.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Wi-Fi 설정 (기본 값)
String wifi_ssid = "";
String wifi_password = "";
bool wifi_received = false;

// MQTT 설정
const char* mqtt_server = "broker.hivemq.com";  // 공개 MQTT 브로커
WiFiClient espClient;
PubSubClient client(espClient);

// Nordic UART Service (NUS) UUIDs
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  // NUS 서비스 UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // RX 특성 UUID
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // TX 특성 UUID

BLECharacteristic *pCharacteristicTX;  // TX 특성 (클라이언트로 데이터 전송)
bool deviceConnected = false;          // BLE 장치 연결 상태 플래그
String rxValue = "";                   // 수신한 데이터 저장

// Wi-Fi 연결 함수
void setup_wifi(const char* ssid, const char* password) {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());  // ESP32의 IP 주소 출력
}

// MQTT 메시지 처리 콜백 함수
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// MQTT 연결 함수
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("test/topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// BLE 서버 콜백 클래스 (BLE 연결 상태 처리)
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE device connected");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE device disconnected");
  }
};

// RX 특성 콜백 클래스 (데이터 수신 처리)
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    // 수신된 데이터를 바로 String으로 변환
    rxValue = pCharacteristic->getValue().c_str();

    if (rxValue.length() > 0) {
      Serial.print("Received over BLE: ");
      Serial.println(rxValue);  // 수신된 데이터를 시리얼 출력

      // 수신된 데이터를 ';'로 구분하여 SSID와 Password로 분리
      int delimiterIndex = rxValue.indexOf(';');
      if (delimiterIndex != -1) {
        wifi_ssid = rxValue.substring(0, delimiterIndex);
        wifi_password = rxValue.substring(delimiterIndex + 1);
        Serial.printf("Extracted SSID: %s, Password: %s\n", wifi_ssid.c_str(), wifi_password.c_str());

        // Wi-Fi 연결 시도
        wifi_received = true;  // Wi-Fi 정보가 수신되었음을 표시
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work with NUS...");

  // BLE 초기화
  BLEDevice::init("ESP32_NUS_UART");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Nordic UART Service (NUS) 설정
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // TX 특성 생성 (Notify로 설정)
  pCharacteristicTX = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pCharacteristicTX->addDescriptor(new BLE2902());

  // RX 특성 생성 (Write로 설정)
  BLECharacteristic *pCharacteristicRX = pService->createCharacteristic(
                                           CHARACTERISTIC_UUID_RX,
                                           BLECharacteristic::PROPERTY_WRITE
                                         );
  pCharacteristicRX->setCallbacks(new MyCharacteristicCallbacks());

  // BLE 서비스 시작
  pService->start();

  // BLE 광고 시작
  pServer->getAdvertising()->start();
  Serial.println("Waiting for a client connection to notify...");

  // MQTT 설정
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  // Wi-Fi 정보가 BLE로 수신되었을 때 Wi-Fi 연결 시도
  if (wifi_received) {
    setup_wifi(wifi_ssid.c_str(), wifi_password.c_str());
    wifi_received = false;  // Wi-Fi 연결 시도 후 플래그 리셋
  }

  // MQTT 연결 상태 확인
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  }

  // BLE 장치 연결 여부 확인 및 데이터 전송 가능
  if (deviceConnected) {
    // TX 특성을 통해 클라이언트로 데이터를 송신
    pCharacteristicTX->setValue("Hello from ESP32!");  // 송신할 데이터 설정
    pCharacteristicTX->notify();  // 데이터를 Notify로 전송
    delay(1000);  // 데이터 전송 주기 (1초)
  }
}
