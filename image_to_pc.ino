#include <SoftwareSerial.h>
#include <fpm.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

//ディスプレイpin
#define TFT_MISO 12
#define TFT_SCK 13
#define TFT_MOSI 11
#define TFT_DC 10
#define TFT_RST 8
#define TFT_CS 9

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST, TFT_MISO);

//指紋センサpin
//in2 is TX,pin3 is RX
SoftwareSerial fserial(2, 3);
FPM finger(&fserial);
FPMSystemParams params;

/* for convenience */
#define PRINTF_BUF_SZ 60
char printfBuf[PRINTF_BUF_SZ];
String text;
int angle = 0;

void setup() {
  randomSeed(analogRead(0));
  angle = random(360);
  Serial.begin(115200);  //speed for usb/pc, maximum measured at 115200. need set tha same setting as pc.
  fserial.begin(57600);  //speed for fingerprint sensor, only at 57600.
  tft.begin();           //Diplay setup

  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(0, 60);
  tft.setTextSize(3);
  tft.println("Hi! I'm\n");
  tft.setCursor(70, 100);
  tft.setTextSize(5);
  tft.setTextColor(ILI9341_YELLOW);
  tft.println("Aketaro");
  Serial.println("Hi! I'm Aketaro");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(0, 200);
  tft.setTextSize(1);
  if (finger.begin()) {
    finger.readParams(&params);
    tft.setTextColor(ILI9341_BLUE);
    tft.println("\n\nFound fingerprint sensor!\nstarting...");
    Serial.println("Found fingerprint sensor!");
  } else {
    tft.setTextColor(ILI9341_RED);
    tft.println("\n\nDid not find fingerprint sensor :(\ncheck the sensor!");
    Serial.println("Did not find fingerprint sensor :(");
    while (1) yield();
  }
  tft.setTextColor(ILI9341_WHITE);
}


void loop() {
  for (int jt = 0; jt < 200; jt++) {  //コマンドの受付
    delay(100);
    text = receive();
  };

  if (text == "send_finger") {  //コマンドの処理
    imageToPc();
  } else {
    angle %= 360;
    testMovingEyes(angle);
    angle += 60;
  }
}


//コマンド受信部
String receive() {
  if (0 < Serial.available()) {
    // 終了文字まで取得
    text = Serial.readStringUntil('\n');
    Serial.println(text);
  }
  return text;
}


uint32_t imageToPc(void) {
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.fillScreen(ILI9341_BLACK);
  FPMStatus status;

  /* Take a snapshot of the finger */
  tft.println("\nPlace a finger.");
  Serial.println("\r\nPlace a finger.");

  int i = 0;
  do {
    status = finger.getImage();
    switch (status) {
      case FPMStatus::OK:
        tft.println("\n\nImage taken!");
        Serial.println("\nImage taken.");
        break;

      case FPMStatus::NOFINGER:
        if (i < 101) {
          if (i % 5 == 0) {
            tft.print(".");
            Serial.print(".");
            i += 1;
          } else {
            i += 1;
          };
        };
        break;

      default:
        /* allow retries even when an error happens */
        tft.println("");
        Serial.println("");
        snprintf(printfBuf, PRINTF_BUF_SZ, "getImage(): error 0x%X", static_cast<uint16_t>(status));
        Serial.println(printfBuf);
        break;
    }
    yield();
  } while (status != FPMStatus::OK);

  tft.println("\nStarting image stream...");
  /* Initiate the image transfer */
  status = finger.downloadImage();

  switch (status) {
    case FPMStatus::OK:
      //tft.println("Starting image stream..."); こいつがいるとReadTimeOutが出るのでdownloadImageの前へ
      Serial.println("Starting image stream...");
      break;

    default:
      snprintf(printfBuf, PRINTF_BUF_SZ, "downloadImage(): error 0x%X", static_cast<uint16_t>(status));
      Serial.println(printfBuf);
      return 0;
  }
  /* Send some arbitrary signature to the PC, to indicate the start of the image stream */
  Serial.write(0xAA);
  uint32_t totalRead = 0;
  uint16_t readLen = 0;

  /* Now, the sensor will send us the image from its image buffer, one packet at a time.
     * We will stream it directly to Serial. */
  bool readComplete = false;

  while (!readComplete) {
    bool ret = finger.readDataPacket(NULL, &Serial, &readLen, &readComplete);

    if (!ret) {
      snprintf_P(printfBuf, PRINTF_BUF_SZ, PSTR("readDataPacket(): failed after reading %u bytes"), totalRead);
      Serial.println(printfBuf);
      return 0;
    }
    totalRead += readLen;
    yield();
  }
  Serial.println();
  Serial.print(totalRead);
  Serial.println(" bytes transferred.");
  tft.println("Transferred!");
  tft.println("\n\nWait for generate pass.");

  text="";
  while(text.length()==0){
    text = receive();
    delay(100);
  }
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 60);
  tft.setTextSize(3);
  tft.println("Password is\n");
  tft.setCursor(0, 100);
  tft.setTextSize(5);
  tft.println(text);
  delay(5000);

  return totalRead;
}


// 目を描く関数
void drawEye(int x, int y, int size, int pupilOffsetX, int pupilOffsetY) {
  int pupilSize = size / 4;  // 瞳のサイズは目のサイズの1/4とする
  int pupilX = x + pupilOffsetX;
  int pupilY = y + pupilOffsetY;
  // 目 (外側の円)
  tft.fillCircle(x, y, size, ILI9341_WHITE);
  // 瞳 (内側の小さい円)
  tft.fillCircle(pupilX, pupilY, pupilSize, ILI9341_BLACK);
}

// アニメーションをテストする関数
void testMovingEyes(int angle) {
  int eyeSize = 50;
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;
  int eyeSpacing = 200;    // 目の間隔
  int movementRange = 30;  // 瞳の動く範囲
  int pupilOffsetX = cos(radians(angle)) * movementRange;
  int pupilOffsetY = sin(radians(angle)) * movementRange;
  tft.setCursor(0, 0);
  tft.fillScreen(ILI9341_BLACK);  // 画面をクリア
  // 左の目
  drawEye(centerX - eyeSpacing / 2, centerY, eyeSize, pupilOffsetX, pupilOffsetY);
  // 右の目
  drawEye(centerX + eyeSpacing / 2, centerY, eyeSize, pupilOffsetX, pupilOffsetY);
}
