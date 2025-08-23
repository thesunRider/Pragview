#include <WiFi.h>
#include "ESP32FtpServer.h"

const char* ssid = "AKPU_2.4GHz";
const char* password = "Kingpin007";

FtpServer ftpSrv;

// SD card options
#define SD_CS 5

void setup(void) {
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  pinMode(17, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  digitalWrite(17, HIGH);
  digitalWrite(16, HIGH);
  digitalWrite(SD_CS, LOW);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //FTP Setup, ensure SD is started before ftp;
  if (SD.begin(SD_CS)) {
    Serial.println("SD opened!");
    //username, password for ftp.  set ports in ESP32FtpServer.h  (default 21, 50009 for PASV)
    ftpSrv.begin("esp32", "esp32");
  } else {
    Serial.println("SD open failed!");
  }
}

void loop(void) {
  ftpSrv.handleFTP();
}
