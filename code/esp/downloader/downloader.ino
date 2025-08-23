#include <WiFi.h>
#include <SD.h>
#include <SPI.h>

const char* ssid = "AKPU_2.4GHz";
const char* password = "Kingpin007";

const char* fileURL = "http://192.168.0.112:8000";  // URL of the file
const char* fileName = "/test.jpg";                 // File name on SD card

#define SD_CS 5  // Change this to your SD card CS pin
SPIClass touchscreenSPI = SPIClass(VSPI);

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  pinMode(17, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  digitalWrite(17, HIGH);
  digitalWrite(16, HIGH);
  digitalWrite(SD_CS, LOW);

}

void loop() {
}