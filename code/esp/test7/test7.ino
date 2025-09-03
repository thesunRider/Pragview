#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "ESP32OTAPull.h"
#include <WiFiClientSecure.h>

#define JSON_URL "https://github.com/thesunRider/Pragview/releases/download/binary_release/config.json"  // this is where you'll post your JSON filter file
String version = "1.2.0";


ESP32OTAPull ota;

struct HttpResult {
  int responseCode;
  size_t contentLength;
};

void setup() {
  Serial.begin(115200);
  WiFi.begin("AKPU_2.4GHz", "Kingpin007");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate check

  String url = "https://github.com/thesunRider/Pragview/releases/download/binary_release/config.json";
  HttpResult result = followRedirects(client, url, 5);

  Serial.printf("\nFinal Response Code: %d\n", result.responseCode);
  Serial.printf("Content Length: %u bytes\n", result.contentLength);
  ota.EnableSerialDebug();

  int ret = ota.CheckForOTAUpdate(&client, JSON_URL, version.c_str(), ESP32OTAPull::UPDATE_BUT_NO_BOOT);

}


void loop() {
  // put your main code here, to run repeatedly:
  delay(10);  // this speeds up the simulation
}

HttpResult followRedirects(WiFiClientSecure &client, String url, int maxRedirects) {
  HttpResult result = {0, 0};

  for (int i = 0; i < maxRedirects; i++) {
    String host, path;
    int port;
    if (!parseURL(url, host, path, port)) {
      Serial.println("Invalid URL: " + url);
      result.responseCode = -1;
      return result;
    }

    Serial.println("Connecting to: " + host);
    if (!client.connect(host.c_str(), port)) {
      Serial.println("Connection failed");
      result.responseCode = -2;
      return result;
    }

    // Send request
    client.print(String("GET ") + path + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: ESP32\r\n" +
                 "Connection: close\r\n\r\n");

    // Read status line
    String statusLine = client.readStringUntil('\n');
    Serial.println(statusLine);
    int statusCode = parseStatusCode(statusLine);

    // Read headers
    String location = "";
    size_t contentLength = 0;
    while (true) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;  // End of headers
      Serial.println(line);
      if (line.startsWith("Location: ")) {
        location = line.substring(10);
        location.trim();
      }
      if (line.startsWith("Content-Length: ")) {
        contentLength = line.substring(16).toInt();
      }
    }

    // If redirect, follow new URL
    if (statusCode >= 300 && statusCode < 400 && location.length() > 0) {
      Serial.println("Redirecting to: " + location);
      url = location;
      client.stop();
      continue;
    }

    // Final response
    result.responseCode = statusCode;
    result.contentLength = contentLength;

    // Print body (stream)
    Serial.println("Body:");
    while (client.available()) {
      Serial.write(client.read());
    }

    break;
  }

  return result;
}

bool parseURL(const String& url, String &host, String &path, int &port) {
  if (!url.startsWith("http")) return false;
  int schemeEnd = url.indexOf("://");
  int pathStart = url.indexOf('/', schemeEnd + 3);
  host = url.substring(schemeEnd + 3, pathStart);
  path = url.substring(pathStart);
  port = url.startsWith("https") ? 443 : 80;
  return true;
}

int parseStatusCode(const String& statusLine) {
  int firstSpace = statusLine.indexOf(' ');
  int secondSpace = statusLine.indexOf(' ', firstSpace + 1);
  return statusLine.substring(firstSpace + 1, secondSpace).toInt();
}