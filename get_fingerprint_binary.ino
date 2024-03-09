#include <SoftwareSerial.h>

// センサーとの通信に使用するピンを定義
int rxPin = 0; // RX Pin
int txPin = 1; // TX Pin

// センサーとの通信用のSoftwareSerialインスタンスを作成
SoftwareSerial fingerprintSensor(rxPin, txPin);

void setup() {
  // PCとのシリアル通信を開始
  Serial.begin(9600);
  // センサーとのシリアル通信を開始
  fingerprintSensor.begin(57600);

  Serial.println("指紋センサー準備完了");

  // 指紋画像をスキャンするコマンドをセンサーに送信
  // GenImgコマンド: 0xEF01FFFFFFFF01000301xx (xxはチェックサム)
  byte genImgCommand[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x01, 0x00, 0x05};
  for (int i = 0; i < sizeof(genImgCommand); i++) {
    fingerprintSensor.write(genImgCommand[i]);
  }
}

void loop() {
  // センサーからデータがある場合、それを読み取りシリアルモニターに出力
  if (fingerprintSensor.available()) {
    Serial.write(fingerprintSensor.read());
  }
}
