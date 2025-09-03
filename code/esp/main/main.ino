#include <TFT_eSPI.h>  // Graphics and font library for ILI9341 driver chip
#include <SPI.h>
#include <ezButton.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <Preferences.h>
#include "time.h"
#include <FS.h>
#include <SD.h>
#include <JpegFunc.h>
#include "ESP32FtpServer.h"
#include "OpenMenuOS.h"
#include <SimpleTimer.h>
#include "WatchedVar.h"
#include "ESP32OTAPull.h"
#include <WiFiClientSecure.h>
#include <IniFile.h>
//meunu
// Static: Network - Connected/Disconnected
//Wifi - On/Off
//Display Clock
//Sync Clock - Last Sync:
//Set Timezone - CET
//Set Photo Change Interval
//Enable Light Sensor
//Set Ambient Light intensity
//Enable Tilt sensor
//Display only Portait on tilted screen and viceversa
//Select photo source - SD/Google photos
//Current Gphotos ID:
//Current Gphotos Album Selected:
//Remove downloaded cloud photos after - 10 days
//Clear storage
//Storage capacity: 10GB/16GB Used
//about
#define JSON_URL "https://github.com/thesunRider/Pragview/releases/download/binary_release/config.json"  // this is where you'll post your JSON filter file
String version = "1.6.0";

// Touchscreen pins
#define XPT2046_IRQ 21   // T_IRQ
#define XPT2046_MOSI 23  // T_DIN
#define XPT2046_MISO 19  // T_OUT
#define XPT2046_CLK 18   // T_CLK
#define XPT2046_CS 17    // T_CS
#define SDCARD_CS 5

#define DISPLAY_LANDSCAPE 3
#define DISPLAY_PORTRAIT 2
int DISPLAY_ORIENTATION = DISPLAY_LANDSCAPE;


SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
//Preferences preferences;
String wifi_configured_ssid;
String wifi_configured_password;


const char *ntpServer = "pool.ntp.org";
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

#define DEFAULT_WIFI_NAME "pragview" //"AKPU_2.4GHz"//"pragview"
#define DEFAULT_WIFI_PASS "123456789" //"Kingpin007" //"123456789"

FtpServer ftpSrv;

#define TFT_GREY 0x5AEB  // New colour
//TFT_eSPI tft = TFT_eSPI();  // Invoke library

String name[20];
String tmp;
int numTabs = 0;
int number = 1;
File root;

ezButton TOUCH1(TOUCH1_PIN);  // create ezButton object that attach to pin 7;
ezButton TOUCH2(TOUCH2_PIN);
ezButton TILT_SNS(TILT_SENS);

// Create menu system instance
OpenMenuOS menu(TOUCH2_PIN, -1, TOUCH1_PIN);

MenuScreen mainMenu("Main Menu");
SettingsScreen SET_networkscreen("Network Settings");
SettingsScreen SET_clockscreen("Clock Settings");
SettingsScreen SET_devicescreen("Device Settings");
SettingsScreen SET_cloudscreen("Cloud Photos Settings");


bool SET_NETWORKSTATUS = false;
bool SET_FTPSTATUS = false;
bool SET_SHOWCLOCK = false;
bool SET_wifipower = true;
const char *tmz[] = { "CET", "IND", "JPN" };

int uid_ftp_connected = 0;
int uid_network_connected = 0;
int uid_dark_intensity = 0;
bool browsing_menu = false;


uint32_t targetTime = 0;         // for next 1 second timeout
uint8_t hh = 0, mm = 0, ss = 0;  // Get H, M, S from compile time

byte omm = 99, oss = 99;
byte xcolon = 0, xsecs = 0;
unsigned int colour = 0;

bool return_home = false;



SimpleTimer slideshow_tmr(10000);
SimpleTimer light_tmr(1000);
SimpleTimer clock_tmr(1000);


int DARK_LIGHT_INTENSITY = 2000;
int intensity_count = 0;
int LIGHT_DURATION = 10;
float average_intensity;
bool light_sensor_enabled = true;
bool titl_sensor_enabled = true;
int dip_timeout;
int light_sensor_theshold_percent = ((1 - (DARK_LIGHT_INTENSITY / 4096)) * 100);
String storage_string;
File entry_jpeg;
String filename;
ESP32OTAPull ota;

void back_tohome();
void showAbout();
void switch_wifi(int val);
void ftp_server_change(int v);
bool listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void init_network_menu();
void intialize_menu();
void mount_sdcard();
bool check_wifi_devices();
void update_displayorientation();
void image_slideshow();
void apply_menu_settings();
void clock_sync();
void clean_storage();
void dark_intensity_sample();
void ota_update();
void ota_callback(int offset, int totallength);
const char *errtext(int code);
void load_validate_ini();
void createIniFile(const char* path);

WatchedVar wifi_power_settings(WiFi.getMode() == WIFI_OFF, switch_wifi);
WatchedVar light_intensity_settings(0, ftp_server_change);

void ota_callback(int offset, int totallength) {
  Serial.printf("Updating %d of %d (%02d%%)...\n", offset, totallength, 100 * offset / totallength);
  static int status = LOW;
  status = status == LOW && offset < totallength ? HIGH : LOW;
  digitalWrite(LED_PIN, status);
}

const char *errtext(int code) {
  switch (code) {
    case ESP32OTAPull::UPDATE_AVAILABLE:
      return "An update is available but wasn't installed";
    case ESP32OTAPull::NO_UPDATE_PROFILE_FOUND:
      return "No profile matches";
    case ESP32OTAPull::NO_UPDATE_AVAILABLE:
      return "Profile matched, but update not applicable";
    case ESP32OTAPull::UPDATE_OK:
      return "An update was done, but no reboot";
    case ESP32OTAPull::HTTP_FAILED:
      return "HTTP GET failure";
    case ESP32OTAPull::WRITE_ERROR:
      return "Write error";
    case ESP32OTAPull::JSON_PROBLEM:
      return "Invalid JSON";
    case ESP32OTAPull::OTA_UPDATE_FAIL:
      return "Update fail (no OTA partition?)";
    default:
      if (code > 0)
        return "Unexpected HTTP response code";
      break;
  }
  return "Unknown error";
}


void ota_update() {
  if (WiFi.status() == WL_CONNECTED) {
    PopupConfig config;
    config.message = "Checking and Installing Update ...";
    config.title = "Starting OTA";
    config.type = PopupType::INFO;
    config.autoClose = true;
    config.autoCloseDelay = 1000;  // 5 seconds
    config.showButtons = false;
    config.showCancelButton = false;
    PopupResult result = PopupManager::show(config);
    PopupResult popupResult = PopupManager::update();
    menu.loop();
    
    //free memory
    entry_jpeg.close();
    root.close();

    WiFiClientSecure client;
    client.setInsecure();  // Skip certificate check

    ota.EnableSerialDebug();
    int ret = ota.CheckForOTAUpdate(&client, JSON_URL, version.c_str(), ESP32OTAPull::UPDATE_AND_BOOT);
    Serial.printf("CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));

    config.message = errtext(ret);
    config.title = "OTA Update Error";
    config.type = PopupType::ERROR;
    config.autoClose = true;
    config.autoCloseDelay = 5000;  // 5 seconds
    config.showButtons = false;
    config.showCancelButton = false;
    result = PopupManager::show(config);
    popupResult = PopupManager::update();
    menu.loop();
    delay(1000);
    ESP.restart();

  } else {
    PopupConfig config;
    config.message = "Not connected to network ...";
    config.title = "Connection Failure";
    config.type = PopupType::ERROR;
    config.autoClose = true;
    config.autoCloseDelay = 3000;  // 5 seconds
    config.showButtons = false;
    config.showCancelButton = false;
    PopupResult result = PopupManager::show(config);
    PopupResult popupResult = PopupManager::update();
    menu.loop();
  }

}

void dark_intensity_sample() {
  PopupConfig config;
  config.message = "Turn of all lights for 10 seconds ...";
  config.title = "Sampling Dark light";
  config.type = PopupType::INFO;
  config.autoClose = true;
  config.autoCloseDelay = 10000;  // 5 seconds
  config.showButtons = false;
  config.showCancelButton = false;
  PopupResult result = PopupManager::show(config);
  PopupResult popupResult = PopupManager::update();


  int sampled_brightness = 0;
  for (int i = 0; i < 10; i++) {
    sampled_brightness += analogRead(LIGHT_SENS);
    PopupManager::update();
    menu.loop();
    delay(1000);
  }
  DARK_LIGHT_INTENSITY = sampled_brightness / 10;
  light_sensor_theshold_percent = (((4096 - DARK_LIGHT_INTENSITY) / 41));
  Serial.printf("New intensity: %d %d \n", light_sensor_theshold_percent, DARK_LIGHT_INTENSITY);
  SET_devicescreen.getelement(uid_dark_intensity)->rangeValue = light_sensor_theshold_percent;
  menu.loop();
}

bool listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  int jpeg_w, jpeg_h;

  while (true) {
    entry_jpeg = root.openNextFile();
    if (!entry_jpeg) { break; }

    filename = entry_jpeg.name();
    filename = "/" + filename;

    // Convert to lowercase to handle .JPG, .JPEG, etc.
    filename.toLowerCase();
    if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) {
      Serial.println(entry_jpeg.name());  // Only JPEG files
      jpegsize(filename.c_str(), &jpeg_w, &jpeg_h);
      if (SET_devicescreen.getSettingValue("Display only Portrait on Tilt")) {
        if (jpeg_w > jpeg_h && DISPLAY_ORIENTATION == DISPLAY_PORTRAIT) {
          continue;
        }

        if (jpeg_w < jpeg_h && DISPLAY_ORIENTATION == DISPLAY_LANDSCAPE) {
          continue;
        }
      }

      return true;
    }
    entry_jpeg.close();
  }
  return false;
}

void clean_storage() {
}

void clock_sync() {
  if (check_wifi_devices()) {
    PopupConfig config;
    config.message = "Time sync Successful ...";
    config.title = "Time Synced";
    config.type = PopupType::INFO;
    config.autoClose = true;
    config.autoCloseDelay = 4000;  // 5 seconds
    config.showButtons = false;
    config.showCancelButton = false;
    PopupResult result = PopupManager::show(config);
    PopupResult popupResult = PopupManager::update();
  } else {
    PopupConfig config;
    config.message = "Couldnt connect to any networks ...";
    config.title = "Conneciton Failure";
    config.type = PopupType::ERROR;
    config.autoClose = true;
    config.autoCloseDelay = 4000;  // 5 seconds
    config.showButtons = false;
    config.showCancelButton = false;
    PopupResult result = PopupManager::show(config);
    PopupResult popupResult = PopupManager::update();
  }
  menu.loop();
}

void back_screen() {
  menu.navigateBack();
}

void back_tohome() {
  browsing_menu = false;
  return_home = true;
}

void showAbout() {
  String info_out = "Pragview v" + version + " -  Digital Photoframe , by Surya for Pragati Mali.. ";
  PopupManager::showInfo(info_out.c_str(), "About");
}

void switch_wifi(int val) {
  SET_wifipower = !(WiFi.getMode() == WIFI_OFF);
  Serial.println("Switching wifi");
  if (val == 0) {
    Serial.println("OFF");
    SET_wifipower = false;
    WiFi.disconnect(true);  // Disconnect from any AP, erase config
    WiFi.mode(WIFI_OFF);
    SET_NETWORKSTATUS = (WiFi.status() == WL_CONNECTED);
    SET_networkscreen.modify(SET_NETWORKSTATUS ? "Network Status > Connected" : "Network Status > Disconnected", uid_network_connected);
    menu.loop();

  } else {
    PopupConfig config;
    config.message = "Scanning for configured Wifi Networks ...";
    config.title = "Connecting Wifi";
    config.type = PopupType::INFO;
    config.autoClose = true;
    config.autoCloseDelay = 5000;  // 5 seconds
    config.showButtons = false;
    config.showCancelButton = false;
    PopupResult result = PopupManager::show(config);
    PopupResult popupResult = PopupManager::update();
    menu.loop();

    Serial.println("ON");
    SET_wifipower = true;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (check_wifi_devices()) {
      PopupConfig config;
      config.message = "Succesfully established Wifi Connection ...";
      config.title = "Connected Wifi";
      config.type = PopupType::INFO;
      config.autoClose = true;
      config.autoCloseDelay = 2000;  // 5 seconds
      config.showButtons = false;
      config.showCancelButton = false;
      PopupResult result = PopupManager::show(config);
      PopupResult popupResult = PopupManager::update();
      SET_NETWORKSTATUS = (WiFi.status() == WL_CONNECTED);
      SET_networkscreen.modify(SET_NETWORKSTATUS ? "Network Status > Connected" : "Network Status > Disconnected", uid_network_connected);
    } else {
      PopupConfig config;
      config.message = "Couldnt connect to any networks ...";
      config.title = "Conneciton Failure";
      config.type = PopupType::ERROR;
      config.autoClose = true;
      config.autoCloseDelay = 4000;  // 5 seconds
      config.showButtons = false;
      config.showCancelButton = false;
      PopupResult result = PopupManager::show(config);
      PopupResult popupResult = PopupManager::update();
    }
    menu.loop();
  }
}

void ftp_server_change(int v) {
  if (SET_networkscreen.getSettingValue("FTP Server:")) {
    tft.setCursor(DISPLAY_ORIENTATION == DISPLAY_PORTRAIT ? 20 : 40, 50, 2);
    // Set the font colour to be white with a black background, set text size multiplier to 1
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.print("FTP SERVER ACTIVE ");
    tft.setTextSize(1);
    tft.setCursor(DISPLAY_ORIENTATION == DISPLAY_PORTRAIT ? 20 : 35, 87, 2);
    String wifip = WiFi.localIP().toString();
    wifip.replace(" ", ".");
    String ftpstr = "ftp://pragview:1234@" + wifip + ":21/ ";
    Serial.println(ftpstr);
    tft.print(ftpstr.c_str());
  }
}


static int jpegDrawCallback(JPEGDRAW *pDraw) {
  // if (pDraw->y >= tft.height()) return 0;
  tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
  return 1;
}

void createIniFile(const char* path) {
  // Check if file exists; remove it first
  if (SD.exists(path)) {
    SD.remove(path);
  }

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file");
    return;
  }

  // Write some settings in INI format
  file.println("[Network]");
  file.println("ssid=nonewifi");
  file.println("password=123456789");

  file.println("[Display]");
  file.println("orientation=landscape");
  file.println("brightness=200");

  file.close();
  Serial.println("INI file created successfully");
}

void load_validate_ini(){
  IniFile ini("/config.ini");
  if (!ini.open()) {
    Serial.print("Ini file does not exist");
    createIniFile("/config.ini");
  }
  Serial.println("Ini file exists");
  const size_t bufferLen = 80;
  char buffer[bufferLen];

  // Check the file is valid. This can be used to warn if any lines
  // are longer than the buffer.
  if (!ini.validate(buffer, bufferLen)) {
    Serial.print("ini file ");
    Serial.print(" not valid: ");
  }
  
  // Fetch a value from a key which is present
  if (ini.getValue("Network", "ssid", buffer, bufferLen)) {
    Serial.print("section 'network' has an entry 'ssid' with value ");
    Serial.println(buffer);
  }

  wifi_configured_ssid = String(buffer);
  Serial.print("read ssid:");
  Serial.print(wifi_configured_ssid);
  Serial.println("-done");

  if (ini.getValue("Network", "password", buffer, bufferLen)) {
    Serial.print("section 'network' has an entry 'pass' with value ");
    Serial.println(buffer);
  }

  wifi_configured_password = String(buffer);
  Serial.print("read pass:");
  Serial.print(wifi_configured_password);
  Serial.println("-done");
}

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
  digitalWrite(TFT_BL, LOW);

  tft.init();
  tft.setRotation(DISPLAY_ORIENTATION);


  delay(120);


  preferences.begin("pragview", false);
  wifi_configured_ssid = preferences.getString("ssid", "");
  wifi_configured_password = preferences.getString("password", "");
  gmtOffset_sec = preferences.getUInt("gmtoff", 0);

  digitalWrite(LED_PIN, HIGH);

  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);

  touchscreen.setRotation(DISPLAY_ORIENTATION);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("Setup done");

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(DISPLAY_ORIENTATION == DISPLAY_PORTRAIT ? 40 : 100, 50, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.print("Pragview ");
  tft.setCursor(DISPLAY_ORIENTATION == DISPLAY_PORTRAIT ? 155 : 215, 70, 2);
  tft.setTextSize(1);
  tft.print("v");
  tft.print(version);
  tft.setCursor(DISPLAY_ORIENTATION == DISPLAY_PORTRAIT ? 50 : 110, 93, 2);
  tft.print("From Surya by Pragati ..");
  delay(1000);
  digitalWrite(TFT_BL, HIGH);  //start displaying stuff
  mount_sdcard();

  TOUCH1.setDebounceTime(50);
  TOUCH2.setDebounceTime(50);
  TILT_SNS.setDebounceTime(50);
  ftpSrv.begin("pragview", "1234");
  load_validate_ini();
  check_wifi_devices();
  intialize_menu();
  storage_string.reserve(30);
  root = SD.open("/");
  root.rewindDirectory();
  filename.reserve(130);
  ota.EnableSerialDebug();
  ota.SetCallback(ota_callback);

  delay(100);
}


void init_network_menu() {
  SET_NETWORKSTATUS = (WiFi.status() == WL_CONNECTED);
  SET_wifipower = !(WiFi.getMode() == WIFI_OFF);

  SET_networkscreen.addBooleanSetting("WiFi Status:", SET_wifipower);
  SET_networkscreen.addBooleanSetting("FTP Server:", SET_FTPSTATUS);
  uid_network_connected = SET_networkscreen.addSubscreenSetting(SET_NETWORKSTATUS ? "Network Status > Connected" : "Network Status > Disconnected", nullptr, nullptr);
  SET_networkscreen.addSubscreenSetting("back", nullptr, back_screen);
}


void intialize_menu() {
  init_network_menu();

  SET_clockscreen.addBooleanSetting("Display Clock", SET_SHOWCLOCK);
  SET_clockscreen.addOptionSetting("Set Timezone", tmz, 3, 0);
  SET_clockscreen.addSubscreenSetting("Sync Clock", nullptr, clock_sync);
  SET_clockscreen.addSubscreenSetting("back", nullptr, back_screen);

  mainMenu.addItem("Network Settings", &SET_networkscreen);
  mainMenu.addItem("Clock Settings", &SET_clockscreen);
  mainMenu.addItem("Device Settings", &SET_devicescreen);
  mainMenu.addItem("Cloud Photos Settings", &SET_cloudscreen);

  SET_devicescreen.addRangeSetting("Photo Duration", 1, 30, 5, "s");
  SET_devicescreen.addBooleanSetting("Enable Display Timeout", dip_timeout);
  SET_devicescreen.addRangeSetting("Display Timeout", 1, 30, 5, "s");
  SET_devicescreen.addBooleanSetting("Enable Light sensor", light_sensor_enabled);
  SET_devicescreen.addSubscreenSetting("Autosample Dark Intensity", nullptr, dark_intensity_sample);
  uid_dark_intensity = SET_devicescreen.addRangeSetting("Dark Intensity", 1, 100, 20, "%");
  Serial.printf("Token for Intensity %d \n", uid_dark_intensity);
  SET_devicescreen.addBooleanSetting("Enable Tilt sensor", true);
  SET_devicescreen.addBooleanSetting("Display only Portrait on Tilt", titl_sensor_enabled);
  SET_devicescreen.addSubscreenSetting("Clear Storage", nullptr, clean_storage);
  SET_devicescreen.addSubscreenSetting("back", nullptr, back_screen);

  int totalBytes = SD.totalBytes() / (1024 * 1024);
  int usedBytes = SD.usedBytes() / (1024 * 1024);
  storage_string = "Storage " + String(usedBytes) + "/" + String(totalBytes) + " MB";
  Serial.println(storage_string);
  mainMenu.addItem("Update Fimrware", nullptr, ota_update);  //link to firmware update
  mainMenu.addItem("About", nullptr, showAbout);
  mainMenu.addItem(storage_string.c_str(), nullptr, nullptr);
  mainMenu.addItem("Back", nullptr, back_tohome);

  // Apply style and start
  menu.useStylePreset("Rabbit_R1");
  menu.setScrollbar(true);
  menu.setAnimation(true);
  menu.setDisplayRotation(DISPLAY_LANDSCAPE);  // Set display rotation
  menu.setOptimizeDisplayUpdates(true);
  menu.setButtonsMode("high");
  menu.begin(&mainMenu);
  Serial.println("Menu began");
}


void mount_sdcard() {
  digitalWrite(XPT2046_CS, HIGH);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SDCARD_CS, LOW);
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

bool check_wifi_devices() {
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
        configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", ntpServer);
        if (!getLocalTime(&timeinfo)) {
          Serial.println("Failed to obtain time");
        }

        return true;  // return without shutting off wifi
      }
    }
  }
  Serial.println("No connectable wifi ap found");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  return false;
}

void update_displayorientation() {
  int new_orentation = (digitalRead(TILT_SENS) == LOW ? DISPLAY_LANDSCAPE : DISPLAY_PORTRAIT);
  if (new_orentation != DISPLAY_ORIENTATION && SET_devicescreen.getSettingValue("Enable Tilt sensor")) {
    DISPLAY_ORIENTATION = new_orentation;
    tft.init();
    tft.setRotation(DISPLAY_ORIENTATION);
  }
  Serial.printf("Display Orientation: %d \n", DISPLAY_ORIENTATION);
}

void image_slideshow() {
  update_displayorientation();
  tft.fillScreen(TFT_BLACK);
  if (!listDir(SD, "/", 0)) {
    root.rewindDirectory();
    listDir(SD, "/", 0);
  }
  if (filename.length() > 0)
    jpegDraw(filename.c_str(), jpegDrawCallback, true, 0, 0, DISPLAY_ORIENTATION == DISPLAY_LANDSCAPE ? TFT_HEIGHT : TFT_WIDTH, DISPLAY_ORIENTATION == DISPLAY_LANDSCAPE ? TFT_WIDTH : TFT_HEIGHT, true, false);
}

void apply_menu_settings() {
  wifi_power_settings.set(SET_networkscreen.getSettingValue("WiFi Status:"));
  light_intensity_settings.set(SET_devicescreen.getSettingValue("Dark Intensity"));
  light_sensor_enabled = SET_devicescreen.getSettingValue("Enable Light sensor");
}


void loop() {
  // put your main code here, to run repeatedly:
  TOUCH1.loop();
  TOUCH2.loop();
  TILT_SNS.loop();

  if (TOUCH1.getState() || TOUCH2.getState()) {
    average_intensity = 0;
    digitalWrite(TFT_BL, HIGH);
  }

  if (return_home) {
    //just exited menu
    image_slideshow();
    ftp_server_change(0);
    Serial.println("Exited menu");
    return_home = false;
    browsing_menu = false;
    slideshow_tmr.setInterval(SET_devicescreen.getSettingValue("Photo Duration") * 1000);
  }

  if (TOUCH1.getState() && TOUCH2.getState() && !browsing_menu) {
    tft.setRotation(DISPLAY_LANDSCAPE);
    browsing_menu = true;
    canvas.pushSprite(0, 0);
    delay(1000);  //wait for a second
  }

  if (SET_networkscreen.getSettingValue("FTP Server:") == 1) {
    ftpSrv.handleFTP();
  }

  if (browsing_menu) {
    PopupResult popupResult = PopupManager::update();
    apply_menu_settings();
    menu.loop();

  } else {
    if (SET_networkscreen.getSettingValue("FTP Server:") == 1) {
      ftpSrv.handleFTP();
    } else {
      if (slideshow_tmr.isReady() || TOUCH2.isPressed()) {
        image_slideshow();
        slideshow_tmr.reset();

        if (SET_clockscreen.getSettingValue("Display Clock")) {

          if (!getLocalTime(&timeinfo)) {
            Serial.println("Failed to obtain time");
          }
          int hours = timeinfo.tm_hour;
          int minutes = timeinfo.tm_min;
          int seconds = timeinfo.tm_sec;
          tft.setCursor(DISPLAY_ORIENTATION == DISPLAY_PORTRAIT ? 20 : 40, 50, 2);
          tft.setTextSize(1);
          int xpos = DISPLAY_ORIENTATION == DISPLAY_PORTRAIT ? 20 : 40;
          int ypos = 50;
          tft.setTextColor(TFT_WHITE);
          if (hours < 10) xpos += tft.drawChar('0', xpos, ypos, 6);  // Add hours leading zero for 24 hr clock
          xpos += tft.drawNumber(hours, xpos, ypos, 6);
          xpos += tft.drawChar(':', xpos, ypos - 6, 6);
          if (minutes < 10) xpos += tft.drawChar('0', xpos, ypos, 6);  // Add minutes leading zero
          xpos += tft.drawNumber(minutes, xpos, ypos, 6);
          xpos = DISPLAY_ORIENTATION == DISPLAY_PORTRAIT ? 25 : 45;
          xpos += tft.drawNumber(timeinfo.tm_mday, xpos, ypos + 50, 4);
          xpos += tft.drawChar('/', xpos, ypos + 50, 4);
          xpos += tft.drawNumber(timeinfo.tm_mon, xpos, ypos + 50, 4);
        }
      }

      if (light_sensor_enabled) {
        if (light_tmr.isReady()) {
          intensity_count++;
          average_intensity += (analogRead(LIGHT_SENS));
          if (intensity_count > LIGHT_DURATION) {
            Serial.printf("Average light intensity : %f \n", average_intensity / intensity_count);
            if (((average_intensity / intensity_count)) > DARK_LIGHT_INTENSITY - 50) {
              //its dark

              Serial.println("Its dark,BL OFF");
              digitalWrite(TFT_BL, LOW);
            } else {
              //its light
              Serial.println("Its Light,BL ON");
              digitalWrite(TFT_BL, HIGH);
            }
            Serial.printf("Intensity: %d \n", average_intensity);
            intensity_count = 0;
            average_intensity = 0;
          }
          light_tmr.reset();
        }
      }
    }
  }



  if (touchscreen.tirqTouched()) {
    if (touchscreen.touched()) {

      average_intensity = 0;
      digitalWrite(TFT_BL, HIGH);

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