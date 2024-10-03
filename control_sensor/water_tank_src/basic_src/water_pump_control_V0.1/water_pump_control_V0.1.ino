// 핀 설정
const int IN1 = 6;  // HG7881 IN1에 연결
const int IN2 = 16;  // HG7881 IN2에 연결

void setup() {
  // 핀 모드 설정
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  // 시리얼 통신 시작
  Serial.begin(115200);

  // 모터 초기 상태 비활성화
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  Serial.println("Pump control ready. Type 'on' to start and 'off' to stop the pump.");
}

void loop() {
  // 시리얼 모니터에서 명령어 받기
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();  // 공백 제거

    if (command == "on") {
      // 펌프 켜기 (한 방향 회전)
      digitalWrite(IN1, HIGH);  // IN1을 HIGH로 설정
      digitalWrite(IN2, LOW);   // IN2는 LOW로 설정
      Serial.println("Pump is ON");
    }
    else if (command == "off") {
      // 펌프 끄기
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      Serial.println("Pump is OFF");
    }
    else {
      Serial.println("Invalid command. Type 'on' or 'off'.");
    }
  }
}
