
#include <TFT_eSPI.h>  // Graphics and font library for ILI9341 driver chip
#include <SPI.h>
#include <ezButton.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <Preferences.h>
#include "time.h"
#include <FS.h>
#include <SD.h>
#include <JPEGDecoder.h>

// Touchscreen pins
#define XPT2046_IRQ 21   // T_IRQ
#define XPT2046_MOSI 23  // T_DIN
#define XPT2046_MISO 19  // T_OUT
#define XPT2046_CLK 18   // T_CLK
#define XPT2046_CS 17    // T_CS
#define SDCARD_CS 5

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
Preferences preferences;
String wifi_configured_ssid;
String wifi_configured_password;

const char* ntpServer = "pool.ntp.org";
long gmtOffset_sec = 0;
int daylightOffset_sec = 3600;
struct tm timeinfo;

#define LED_PIN 33
#define TOUCH1_PIN 25
#define TOUCH2_PIN 26
#define CHRG_STNDBY 35
#define CHRG_RDY 32
#define TILT_SENS 34
#define BAT_SENS 36
#define LIGHT_SENS 39

#define TFT_HOR_RES 320
#define TFT_VER_RES 240

#define DEFAULT_WIFI_NAME "pragview"
#define DEFAULT_WIFI_PASS "123456789"

#define TFT_GREY 0x5AEB     // New colour
TFT_eSPI tft = TFT_eSPI();  // Invoke library


ezButton TOUCH1(TOUCH1_PIN);  // create ezButton object that attach to pin 7;
ezButton TOUCH2(TOUCH2_PIN);
ezButton TILT_SNS(TILT_SENS);

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  pinMode(LED_PIN, OUTPUT);
  pinMode(TOUCH1_PIN, INPUT);
  pinMode(TOUCH2_PIN, INPUT);
  pinMode(CHRG_STNDBY, INPUT);
  pinMode(CHRG_RDY, INPUT);
  pinMode(TILT_SENS, INPUT);
  pinMode(BAT_SENS, INPUT);
  pinMode(LIGHT_SENS, INPUT);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(3);
  
  

  preferences.begin("pragview", false);
  wifi_configured_ssid = preferences.getString("ssid", "");
  wifi_configured_password = preferences.getString("password", "");
  gmtOffset_sec = preferences.getUInt("gmtoff", 0);
  daylightOffset_sec = preferences.getUInt("dayoff", 3600);  //daylight offsetinsecs

  digitalWrite(LED_PIN, HIGH);

  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);

  touchscreen.setRotation(3);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("Setup done");

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(100, 50, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.print("Pragview ");
  tft.setCursor(215, 70, 2);
  tft.setTextSize(1);
  tft.print("v1.2");
  tft.setCursor(110, 93, 2);
  tft.print("From Surya ....");
  mount_sdcard();
  check_wifi_devices();

  delay(100);
}

void mount_sdcard(){
  if (!SD.begin(SDCARD_CS, touchscreenSPI)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.println("sdcard init finished");
}

void check_wifi_devices() {
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  bool found_wifi = false;
  String wifi_ssid;
  String wifi_password;

  if (n == 0) {
    Serial.println("no networks found");

  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {

      if (WiFi.SSID(i).equals(wifi_configured_ssid)) {
        wifi_ssid = wifi_configured_ssid;
        wifi_password = wifi_configured_password;
        found_wifi = true;
        break;
      }

      if (WiFi.SSID(i).equals(DEFAULT_WIFI_NAME)) {
        wifi_ssid = DEFAULT_WIFI_NAME;
        wifi_password = DEFAULT_WIFI_PASS;
        found_wifi = true;
        break;
      }
    }
  }

  if (found_wifi) {
    WiFi.begin(wifi_ssid, wifi_password);
    Serial.print("Connecting to WiFi - ");
    Serial.print(wifi_ssid);
    for (int i = 0; i < 10; i++) {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
      } else {
        //connected to internet
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        if (!getLocalTime(&timeinfo)) {
          Serial.println("Failed to obtain time");
        }
        return; // return without shutting off wifi
      }
    }

  } 
    Serial.println("No connectable wifi ap found");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void loop() {
  // put your main code here, to run repeatedly:
  TOUCH1.loop();
  TOUCH2.loop();
  TILT_SNS.loop();

  tft.fillScreen(TFT_GREY);

  // Set "cursor" at top left corner of display (0,0) and select font 2
  // (cursor will move to next line automatically during printing with 'tft.println'
  //  or stay on the line is there is room for the text with tft.print)
  tft.setCursor(0, 0, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  // We can now plot text on screen using the "print" class
  tft.print("Touch1=");
  tft.println(TOUCH1.getState());
  tft.print("Touch2=");
  tft.println(TOUCH2.getState());
  tft.print("TILT=");
  tft.println(TILT_SNS.getState());
  tft.print("LIGHT_SENS=");
  tft.println(analogRead(LIGHT_SENS));

  if (touchscreen.tirqTouched()) {
    if (touchscreen.touched()) {
      TS_Point p = touchscreen.getPoint();
      Serial.print("Pressure = ");
      Serial.print(p.z);
      Serial.print(", x = ");
      Serial.print(p.x);
      Serial.print(", y = ");
      Serial.print(p.y);
      delay(30);
      Serial.println();
    }
  }
}
