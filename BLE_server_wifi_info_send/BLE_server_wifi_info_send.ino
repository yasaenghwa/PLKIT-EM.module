#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// BLE UUID 설정
#define SERVICE_UUID        "736D6172-7462-6F61-7264-5F706C6B6974"
#define SSID_CHARACTERISTIC_UUID "5F706C6B-6974-5F77-6966-695F6E616D65"
#define PASSWORD_CHARACTERISTIC_UUID "5F5F706C-6B69-745F-7061-7373776F7264"

// Wi-Fi 정보 (BLE로 전송할 정보)
String ssid = "PLKit";
String password = "987654321";

// BLE 서버 설정
BLECharacteristic *pSSIDCharacteristic;
BLECharacteristic *pPasswordCharacteristic;

void setup() {
  Serial.begin(115200);

  // BLE 초기화
  BLEDevice::init("ESP32_BLE_Server");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // SSID characteristic 생성 및 설정
  pSSIDCharacteristic = pService->createCharacteristic(
                         SSID_CHARACTERISTIC_UUID,
                         BLECharacteristic::PROPERTY_READ
                       );
  pSSIDCharacteristic->setValue(ssid.c_str());

  // Password characteristic 생성 및 설정
  pPasswordCharacteristic = pService->createCharacteristic(
                             PASSWORD_CHARACTERISTIC_UUID,
                             BLECharacteristic::PROPERTY_READ
                           );
  pPasswordCharacteristic->setValue(password.c_str());

  // 서비스 시작
  pService->start();

  // BLE 광고 시작
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
  
  Serial.println("BLE server is running and advertising Wi-Fi credentials...");
}

void loop() {
  // BLE 광고는 자동으로 처리되므로 별도 로직 불필요
}
