#include <SoftwareSerial.h>
#include <fpm.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

//ディスプレイpin
#define TFT_MISO 12
#define TFT_SCK 13
#define TFT_MOSI 11
#define TFT_DC 8
#define TFT_RST 9
#define TFT_CS 10

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST, TFT_MISO);

//指紋センサpin
//in2 is TX,pin3 is RX
SoftwareSerial fserial(2, 3);
FPM finger(&fserial);
FPMSystemParams params;

/* for convenience */
#define PRINTF_BUF_SZ 60
char printfBuf[PRINTF_BUF_SZ];
//String cmd;

void setup() {
  Serial.begin(115200);  //speed for usb/pc, maximum measured at 115200. need set tha same setting as pc.
  fserial.begin(57600);  //speed for fingerprint sensor, only at 57600.
  tft.begin();           //Diplay setup

  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.println("Hi! I'm Aketaro");
  Serial.println("Hi! I'm Aketaro");
  tft.setTextSize(2);

  if (finger.begin()) {
    finger.readParams(&params);
    tft.println("Found fingerprint sensor!");
    Serial.println("Found fingerprint sensor!");
  } else {
    tft.println("Did not find fingerprint sensor :(");
    Serial.println("Did not find fingerprint sensor :(");
    while (1) yield();
  }
}


void loop() {
  //   for(int jt=0; jt<200; jt++){//コマンドの受付
  //   cmd = receive();
  //   delay(100);
  // };

  // if(cmd=="send_finger"){//コマンドの処理
  //     imageToPc();
  //     while (1) yield();
  // };
  imageToPc();
  while (1) yield();
}


//コマンド受信部
// String receive() {
//   if (0 < Serial.available()) {
//     // 終了文字まで取得
//     cmd = Serial.readStringUntil('\n');
//     Serial.println(cmd);
//     tft.println(cmd);
//   }

//   return cmd;
// }


uint32_t imageToPc(void) {
  FPMStatus status;

  /* Take a snapshot of the finger */
  tft.println("\nPlace a finger.");
  Serial.println("\r\nPlace a finger.");

  int i = 0;
  do {
    status = finger.getImage();
    switch (status) {
      case FPMStatus::OK:
        tft.println("\nImage taken.");
        Serial.println("\nImage taken.");
        break;

      case FPMStatus::NOFINGER:
        if (i < 51) {
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

  tft.println("Starting image stream...");
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
  tft.println("transferred.");
  return totalRead;
}
