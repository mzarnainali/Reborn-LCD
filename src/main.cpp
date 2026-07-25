// https://tomeko.net/online_tools/file_to_hex.php?lang=en  Picture to Hexa Convert

// http://www.rinkydinkelectronics.com/t_imageconverter565.php image to rgb565 converter

// https://icircuit.net/adding-custom-fonts-to-tft_espi/4090 custom font
// https://rop.nl/truetype2gfx/ custom font online convert tool

// https://funstacklabs.github.io/font-rasteriser.html only number font
// jgromes/RadioLib@^7.1.2

// https://cifertech.github.io/DisplayKit/    online tool to generate code for display, can be used to generate code for static screens like trip screen, can also be used to generate code for icons like lock unlock icons, can also be used to generate code for fonts, but it is not good for that, because it does not support anti-aliasing and it generates code for each character separately, which is not good for memory management, but it can be used to generate code for static screens and icons

/*
Speedo Meter UART data decode
*Fuel 1 - 20
Temp 33 - 54 = 54-33-1 => 20
       Speed  Fuel*  Temp
76 42  0      20     54     12 0 0 252 0 0 0 0 0 56 95 161

*/

/*
  LCD Screen Layout

 (0,0) Top-Left [Ribbon Cable / Pins at Top Edge] ─── (320,0) Top-Right
    │                                                    │
    │                                                    │
    │     ► Your tft.fillRect(5, 145, 300, 35...)        │
    │       clears a wide bar right here.                │
    │                                                    │
(0,240) Bottom-Left ───────────────────────────── (320,240) Bottom-Right

*/

/*
  ToDo:
  1. Add average Speed
  2. show idle time with idle fule in litre
  3. show remaning fuel in percentage
  4. show remaning fule cover distance

  5. calculate per dot fuel in litre ans save for calc.

*/

#define BLE 0 // SET TO 0 OUT TO REMOVE TRACES
#define RESET_FLASH 0
#define DEBUG 0 // SET TO 0 OUT TO REMOVE TRACES

#include <Arduino.h>
#include "esp_task_wdt.h"
#include "esp_int_wdt.h"
#include "soc/rtc_wdt.h"
#include "soc/rtc.h"
#include "esp_system.h"

#define TYPE_LCD 1
#define TYPE_DOOR 2

#if BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
// Lock Control BLE Service UUID (example: random but structured)
#define SERVICE_UUID "f9b34fb0-42c0-4f8f-9e09-1cbeb345a123"
// Lock/Unlock Command BLE Characteristic UUID
#define CMD_CHARACTERISTIC_UUID "f9b34fb1-42c0-4f8f-9e09-1cbeb345a123"
#define STATUS_CHARACTERISTIC_UUID "f9b34fb2-42c0-4f8f-9e09-1cbeb345a123"
BLECharacteristic *pCharacteristic;
BLECharacteristic *pStatusCharacteristic;
#endif

#define TIMEOUT_SEC 30
#define APB_CLK 80000000 // 80 MHz default APB clock

#if DEBUG
#define D_SerialBegin(...) Serial.begin(__VA_ARGS__);
#define D_print(...) Serial.print(__VA_ARGS__)
#define D_write(...) Serial.write(__VA_ARGS__)
#define D_println(...) Serial.println(__VA_ARGS__)
#else
#define D_SerialBegin(...)
#define D_print(...)
#define D_write(...)
#define D_println(...)
#endif

#define CAN_RX_PIN 33
#define CAN_TX_PIN 32

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
// #include <WiFiUdp.h>
//  #include <WebSocketsServer.h>
// #include <Update.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "Button2.h"
#include <JPEGDecoder.h>
#include "dua.h"
#include "car.h"
#include "car_with_tyre.h"
#include "door_fl.h"
#include "door_fr.h"
#include "door_rl.h"
#include "door_rr.h"
#include "door_back.h"
#include "meter_icons.h"
#include "FreeSans36pt7b.h"
#include <RTClib.h>
#include <Preferences.h>
#include <Adafruit_MCP23X17.h>
#include <RadioLib.h>
#include "Wire.h"
#include "SHT2x.h"
#include <HardwareSerial.h>

#include <esp32_can.h> // https://github.com/collin80/esp32_can and https://github.com/collin80/can_common CAN libraries
#include <esp32_obd2.h>

#include <main_page.h>
#include <obd_main_page.h>
#include <global.h>
#include <helper.h>

#if RESET_FLASH
#include "nvs_flash.h"
void resetFlash()
{
  Serial.begin(115200);
  Serial.println("Trying to erse all NVS flash...");
  if (nvs_flash_erase() != ESP_OK)
    Serial.println("===> Error with Flash Erase.");
  if (nvs_flash_init() != ESP_OK)
    Serial.println("===> Error with Flash INIT.");
  Serial.println("Done with NVS flash...");
  delay(1410065407);
}
#endif

// const char *serverIndex = "<meta name='viewport' content='width=device-width, initial-scale=1.0'><form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>\n    <input type='file' name='update'>\n    <input type='submit' value='Update'>\n</form>\n<div id='prg'>progress: 0%</div>\n<script>\ndocument.getElementById('upload_form').addEventListener('submit', function(e) {\n    e.preventDefault();\n    \n    var form = document.getElementById('upload_form');\n    var data = new FormData(form);\n\n    var xhr = new XMLHttpRequest();\n    xhr.open('POST', '/ota_update', true);\n\n    xhr.upload.addEventListener('progress', function(evt) {\n        if (evt.lengthComputable) {\n            var per = evt.loaded / evt.total;\n            document.getElementById('prg').innerHTML = 'progress: ' + Math.round(per * 100) + '%';\n        }\n    }, false);\n\n    xhr.onload = function() {\n        if (xhr.status === 200) {\n            console.log('success!');\n        }\n    };\n\n    xhr.onerror = function() {\n        // Handle error\n    };\n\n    xhr.send(data);\n});\n</script>";

void lockDoor();
void unLockDoor();
// const char *ssid = "Logout";
int headerHeight = 41;
String previousInstantAvg = "99.99";
static String previousInstantAvgInt = "99.99";
byte previous_kpl_lh = 9;
String previousRunningTime = "99.99";
String previousIdleRunningTime = "99.99";
bool isMcpOk = false;
bool isTPMS = false;
DateTime now;
unsigned long last_tpms_id;
byte last_checksum;
byte totalTrips = 2; // Mean total trips are 3, because of 0 based index, we are setting it to 2, so that it can go up to 3
byte currentTrip = 0;
DNSServer dnsServer;
AsyncWebServer server(80);

AsyncWebSocket webSocket("/ws");

WiFiServer telnetServer(23);
WiFiClient telnetClient;
// WebSocketsServer webSocket = WebSocketsServer(1337);

int tft_W = 0;
int tft_H = 0;
static bool isKhudaHafiz = false;

SPIClass tpmsSPI(VSPI);
CC1101 radio = new Module(CC1101_CS_PIN, CC1101_GDO0_PIN, RADIOLIB_NC, RADIOLIB_NC /*CC1101_GDO2_PIN8*/, tpmsSPI, SPISettings(2000000, MSBFIRST, SPI_MODE0));

RTC_DS3231 rtc;
bool isRTC_Connected = false;
Button2 btnPlus, btnMinus;
SHT2x sht;
Adafruit_MCP23X17 mcp;

// HardwareSerial Serial2(2); // Use UART2
// HardwareSerial Serial3(); // Use UART3

byte step1 = 50;
byte step2 = 102;
byte step3 = 153;
byte step4 = 204;

bool clearScreenReq = false;
bool temporaryScreen = false;
bool updateScreen = true;
bool updateTPMSScreen = false;
bool isServerConnected = false;
unsigned long temporaryScreenMillis = 0;
bool nativeButtonLongClick = false;
long currentTime = 0;
float carTemperature = 0, carHumidity = 0;
int avg_speed = 0;
unsigned long idleCarRunTimeMs = 0; // Total time spent idle (Speed = 0)
unsigned long lastUpdateTime = 0;   // Stores the previous timestamp
float totalMovingTimeMs = 0;        // Total time spent moving (excluding idle)
float totalDistanceKm = 0;          // Accumulated distance during motion
// flag to indicate that a packet was received
volatile bool receivedFlag = false;
// Return the minimum of two values a and b
#define minimum(a, b) (((a) < (b)) ? (a) : (b))
// Esp32 tester MAC Address
// uint8_t broadcastAddress[] = {0xFC, 0xE8, 0xC0, 0x74, 0x2E, 0x4C};
// Esp32 in Reborn MAC Address   48:E7:29:98:B4:1C
uint8_t broadcastAddress[] = {0x48, 0xE7, 0x29, 0x98, 0xB4, 0x1C};
esp_now_peer_info_t peerInfo;
void send_page_number(int tripNo, byte buttonState);

bool isOBDServer = false;
bool isLockByAndriod = false;
bool isLockByAndriodReboot = false;
unsigned long lastLockByAndriodTime = 0;

char daysOfTheWeek[7][12] = {"Sunday",
                             "Monday",
                             "Tuesday",
                             "Wednesday",
                             "Thursday",
                             "Friday",
                             "Saturday"};

char daysOfTheWeekShort[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

const char *monthsOfTheYear[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

typedef struct LCD_Data
{
  byte dataType;
  char gear;
  bool converter_lock;
  float inst_kml;
  float inst_lph;
  byte totalTrips;
  byte currentTrip;
  float trip_distance;
  float trip_avg;
  float trip_fuel;
  float trip_idle_fuel;
  int trip_price;
  float speed;
  int avg_speed = 0;
  float outSideTemp;
  uint16_t rpm;
  uint16_t inj_timing;
  float duty_cycle;
  float inj_freq;
  unsigned long trip_RunTime;
  unsigned long trip_idle_RunTime;
} LCD_Data;

typedef struct Door_Data
{
  byte dataType;
  byte doorStatus;
} Door_Data;

typedef struct LCD_Response
{
  byte tripNo;
  byte buttonState;
} LCD_Response;

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spriteHeader = TFT_eSprite(&tft);
LCD_Response sendBackData;
LCD_Data lcdData;
LCD_Data printedLcdData;
Door_Data doorData;
OBD2_DATA OBD2Data;
Preferences preferences;


void clearScreen()
{
  // tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 42, tft_W, (tft_H - 40), TFT_BLACK);
}
// Helper function signature for screen entry handlers
typedef void (*ScreenHandler)();

// Optional handlers for screens that require special actions on load
void onTPMSEnter() {
  updateTPMSScreen = true;
}

void onTripEnterForward() {
  currentTrip = 0;
  send_page_number(currentTrip, 1);
}

void onTripEnterBackward() {
  currentTrip = totalTrips;
  clearScreen();
  send_page_number(currentTrip, 1);
  clearScreenReq = true;
}

// Data structure to represent screen behavior
struct ScreenConfig {
  Screen screenId;
  const char* name;
  bool hasSubPages;             // Set true for multi-page screens like TRIP
  ScreenHandler onEnterForward;  // Triggered when entering forward (+)
  ScreenHandler onEnterBackward; // Triggered when entering backward (-)
};

// Define menu structure in array sequence order!
// To insert/remove/reorder screens, simply adjust this list.
const ScreenConfig screenList[] = {
  { CURRENT_TIME_SCREEN,    "CURRENT_TIME_SCREEN",    false, nullptr,            nullptr },
  { TPMS_SCREEN,            "TPMS_SCREEN",            false, onTPMSEnter,        onTPMSEnter },
  { TRIP_SCREEN,            "TRIP_SCREEN",            true,  onTripEnterForward, onTripEnterBackward },
  { INJECTOR_TIMING_SCREEN, "INJECTOR_TIMING_SCREEN", false, nullptr,            nullptr },
  { SPEED_SCREEN,           "SPEED_SCREEN",           false, nullptr,            nullptr }
};


const byte NUM_CONFIGURED_SCREENS = sizeof(screenList) / sizeof(screenList[0]);
int currentScreenIndex = 0; // Tracks position inside screenList[]

byte doorStatusCmd = 0;

bool isEngineRunning = false;
bool isEngineStopped = false;
unsigned long engineStartTime = 0;
unsigned long lastZeroTime = 0;
unsigned long previousEngineRunTime = 0;
unsigned long currentEngineRunTime = 0;
bool isPetrolZero = false;
int lcdDataSize = 0;
int doorDataSize = 0;
String PreviousTimeString;

Screen currentScreen = CURRENT_TIME_SCREEN;
Screen previousScreen = CURRENT_TIME_SCREEN;



int countDigits(int number)
{
  if (number < 0)
    number = abs(number); // Handle negative values just in case

  if (number < 10)
    return 1;
  if (number < 100)
    return 2;
  if (number < 1000)
    return 3;
  return 4; // Covers 1000 to 9999 (perfect for Speed and RPM ranges)
}

void initPrvLcdData()
{
  printedLcdData.gear = '?';             // Neutral gear
  printedLcdData.converter_lock = false; // Converter unlocked
  printedLcdData.inst_kml = -1.0f;       // Instantaneous km/l
  printedLcdData.inst_lph = -1.0f;       // Instantaneous l/h
  printedLcdData.totalTrips = -1;        // Total trips
  printedLcdData.currentTrip = -1;       // Current trip index
  printedLcdData.trip_distance = -1.0f;  // Trip distance
  printedLcdData.trip_avg = -1.0f;       // Trip average consumption
  printedLcdData.trip_fuel = -1.0f;      // Trip fuel used
  printedLcdData.trip_price = -1;        // Trip price
  printedLcdData.speed = -1.0f;          // Vehicle speed
  printedLcdData.outSideTemp = -1.0f;
  printedLcdData.rpm = -1;
  printedLcdData.inj_timing = -1;
  printedLcdData.duty_cycle = -1.0f;
  printedLcdData.inj_freq = -1.0f;
}


template <typename T>
// Function to draw a vertical gradient rectangle with a split position.
void drawVerticalGradientRectWithSplit(T &disp, int16_t x, int16_t y, int16_t w, int16_t h,
                                       uint16_t topColor, uint16_t bottomColor, float splitPos)
{
  int16_t splitLine = h * splitPos;
  for (int16_t i = 0; i < h; i++)
  {
    uint16_t color;
    if (i < splitLine)
    {
      color = topColor;
    }
    else
    {
      float ratio = (float)(i - splitLine) / (float)(h - splitLine);
      uint8_t r1 = (topColor >> 11) & 0x1F;
      uint8_t g1 = (topColor >> 5) & 0x3F;
      uint8_t b1 = topColor & 0x1F;

      uint8_t r2 = (bottomColor >> 11) & 0x1F;
      uint8_t g2 = (bottomColor >> 5) & 0x3F;
      uint8_t b2 = bottomColor & 0x1F;

      uint8_t r = r1 + (r2 - r1) * ratio;
      uint8_t g = g1 + (g2 - g1) * ratio;
      uint8_t b = b1 + (b2 - b1) * ratio;

      color = disp.color565((r * 255) / 31, (g * 255) / 63, (b * 255) / 31);
    }
    disp.drawFastHLine(x, y + i, w, color);
  }
}

// Function to draw a vertical gradient rectangle
void drawVerticalGradientRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colorTop, uint16_t colorBottom)
{
  for (int16_t i = 0; i < h; i++)
  {
    // Calculate the interpolation ratio (0.0 to 1.0)
    float ratio = (float)i / (h - 1);

    // Extract the 5-bit/6-bit color components from the 16-bit color values
    uint8_t r1 = (colorTop >> 11) & 0x1F;
    uint8_t g1 = (colorTop >> 5) & 0x3F;
    uint8_t b1 = colorTop & 0x1F;

    uint8_t r2 = (colorBottom >> 11) & 0x1F;
    uint8_t g2 = (colorBottom >> 5) & 0x3F;
    uint8_t b2 = colorBottom & 0x1F;

    // Interpolate each color component
    uint8_t r = r1 + (r2 - r1) * ratio;
    uint8_t g = g1 + (g2 - g1) * ratio;
    uint8_t b = b1 + (b2 - b1) * ratio;

    // Convert back to a 16-bit color value
    uint16_t color = tft.color565((r * 255) / 31, (g * 255) / 63, (b * 255) / 31);

    // Draw one horizontal line of the gradient
    tft.drawFastHLine(x, y + i, w, color);
  }
}
// ####################################################################################################
//  Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
// ####################################################################################################
//  This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
//  fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void renderJPEG(int xpos, int ypos)
{
  /*
    Serial.println(F("==============="));
    Serial.println(F("JPEG image info"));
    Serial.println(F("==============="));
    Serial.print(F("Width      :"));
    Serial.println(JpegDec.width);
    Serial.print(F("Height     :"));
    Serial.println(JpegDec.height);
    Serial.print(F("Components :"));
    Serial.println(JpegDec.comps);
    Serial.print(F("MCU / row  :"));
    Serial.println(JpegDec.MCUSPerRow);
    Serial.print(F("MCU / col  :"));
    Serial.println(JpegDec.MCUSPerCol);
    Serial.print(F("Scan type  :"));
    Serial.println(JpegDec.scanType);
    Serial.print(F("MCU width  :"));
    Serial.println(JpegDec.MCUWidth);
    Serial.print(F("MCU height :"));
    Serial.println(JpegDec.MCUHeight);
    Serial.println(F("==============="));
  */
  // retrieve infomration about the image
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  // uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while (JpegDec.readSwappedBytes())
  {

    // save a pointer to the image block
    pImg = JpegDec.pImage;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos; // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x)
      win_w = mcu_w;
    else
      win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y)
      win_h = mcu_h;
    else
      win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // draw image MCU block only if it will fit on the screen
    if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height())
    {
      tft.pushRect(mcu_x, mcu_y, win_w, win_h, pImg);
    }
    else if ((mcu_y + win_h) >= tft.height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }
}

void readSetting()
{
  D_println("readSetting");
  // dummySetting();
  preferences.begin("zApp", false);
  // Retrieve the stored value; default to CURRENT_TIME_SCREEN if not found
  int storedScreen = preferences.getInt("currentScreen", CURRENT_TIME_SCREEN);
  currentTrip = preferences.getInt("currentTrip", 0);
  tyre_FL_PSI = preferences.getFloat("fl_psi", 0);
  tyre_FR_PSI = preferences.getFloat("fr_psi", 0);
  tyre_RL_PSI = preferences.getFloat("rl_psi", 0);
  tyre_RR_PSI = preferences.getFloat("rr_psi", 0);

  ID_FL = preferences.getULong("id_fl", ID_FL_DEF);
  ID_FR = preferences.getULong("id_fr", ID_FR_DEF);
  ID_RL = preferences.getULong("id_rl", ID_RL_DEF);
  ID_RR = preferences.getULong("id_rr", ID_RR_DEF);

  D_print("ID_FL: ");
  D_println(ID_FL);

  D_print("ID_FR: ");
  D_println(ID_FR);

  D_print("ID_RL: ");
  D_println(ID_RL);

  D_print("ID_RR: ");
  D_println(ID_RR);

  currentScreen = static_cast<Screen>(storedScreen);
  isLockByAndriod = preferences.getBool("isLockByAndriod", false);
  preferences.putBool("isLockByAndriod", false);
  isLockByAndriodReboot = isLockByAndriod;
  preferences.end();
  // send_page_number(currentTrip, 1);
  D_print("currentScreen: ");
  D_println(currentScreen);
}

void printLCDData(const LCD_Data &data)
{
  return;
  D_print("gear: ");
  D_print(data.gear);
  D_print("   converter_lock: ");
  D_print(data.converter_lock ? "true" : "false");
  D_print("   speed: ");
  D_print(data.speed, 2);
  D_print("   inst_kml: ");
  D_print(data.inst_kml, 2);
  D_print("   inst_lph: ");
  D_print(data.inst_lph, 2);
  D_print("   trip_distance: ");
  D_print(data.trip_distance, 2);
  D_print("   trip_avg: ");
  D_print(data.trip_avg, 2);
  D_print("   trip_fuel: ");
  D_print(data.trip_fuel, 2);
  D_print("   trip_price: ");
  D_print(data.trip_price);

  D_print("   totalTrips: ");
  D_print(data.totalTrips);

  D_print("   currentTrip: ");
  D_print(data.currentTrip);

  D_print("   outSideTemp: ");
  D_print(data.outSideTemp);

  D_println(""); // Ensures the output ends in a new line
}
// ####################################################################################################
//  Draw a JPEG on the TFT pulled from a program memory array
// ####################################################################################################
void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos)
{
  int x = xpos;
  int y = ypos;
  if (JpegDec.decodeArray(arrayname, array_size))
  {
    renderJPEG(x, y);
  }
}

void resetVariable()
{
  btnCounter = 0;
  updateScreen = true;
  updateTPMSScreen = true;
  clearScreenReq = true;
  previousInstantAvg = "99.99";
  previousInstantAvgInt = "99.99";
  previous_kpl_lh = -9;
  previousRunningTime = "99.99";
  previousIdleRunningTime= "99.99";
  PreviousTimeString = "-";
  initPrvLcdData();
}

void preferencesOpen()
{
  preferences.begin("zApp", false);
}

void preferencesEnd()
{
  preferences.end();
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  return;
  D_print("\r\nLast Packet Send Status:\t");
  D_println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  byte dataType = incomingData[0]; // First byte is always type
  if (dataType == TYPE_LCD)
  {
    memcpy(&lcdData, incomingData, lcdDataSize);
    currentTrip = lcdData.currentTrip;
    totalTrips = lcdData.totalTrips;
    if (currentScreen == TRIP_SCREEN)
      printLCDData(lcdData);
    updateScreen = true;
    isServerConnected = true;
  }
  else if (dataType == TYPE_DOOR)
  {
    memcpy(&doorData, incomingData, doorDataSize);
    if (doorData.doorStatus == 1)
    {
      lockDoor();
    }
    else if (doorData.doorStatus == 2)
    {
      unLockDoor();
    }
  }
}

String addCommas(long num)
{
  String numStr = String(num); // Convert the number to a string
  int len = numStr.length();   // Get the length of the string
  String result = "";          // Initialize an empty result string

  // Traverse the string from the end to the beginning
  for (int i = 0; i < len; i++)
  {
    // Insert a comma after every third digit (except at the beginning)
    if (i > 0 && (i % 3) == 0)
    {
      result = "," + result;
    }
    // Add the current digit to the result
    result = numStr[len - 1 - i] + result;
  }

  return result; // Return the formatted string
}

void selectScreen(Screen screen)
{
  if (currentScreen != previousScreen)
  {
    previousScreen = currentScreen;
    currentScreen = screen;
  }
}

unsigned int lock_color = 0x7BEF;

String milli2Time(unsigned long totMillis)
{
  unsigned long hours = totMillis / (3600 * 1000);                   // 3,600,000 milliseconds in an hour
  unsigned long minutes = (totMillis % (3600 * 1000)) / (60 * 1000); // Remaining milliseconds converted to minutes
  unsigned long seconds = (totMillis % (60 * 1000)) / 1000;          // Remaining milliseconds converted to seconds

  String tm = "";

  if (hours < 10)
  {
    tm += "0" + String(hours) + ":";
  }
  else
  {
    tm += String(hours) + ":";
  }

  if (minutes < 10)
  {
    tm += "0" + String(minutes);
  }
  else
  {
    tm += String(minutes);
  }

  tm += ":";

  if (seconds < 10)
  {
    tm += "0" + String(seconds);
  }
  else
  {
    tm += String(seconds);
  }

  return tm;
}

String seconds2Time(unsigned long totSeconds)
{
  return milli2Time(totSeconds * 1000);
}

String float2String(float value)
{
  char buffer[8];               // Make sure the buffer is large enough to hold your formatted string
  dtostrf(value, 4, 2, buffer); // Format the float value with 5 characters total, including 2 decimal places
  String formattedValue = String(buffer);
  // Add leading zero if necessary
  if (formattedValue.length() == 4)
  {
    formattedValue = "0" + formattedValue;
  }
  return formattedValue;
}

void printGear(char currentGear, bool torqueLocked)
{

  if (currentGear == '\0')
  {
    currentGear = '*';
  }

  if (currentGear != printedLcdData.gear || torqueLocked != printedLcdData.converter_lock)
  {
    printedLcdData.gear = currentGear;
    printedLcdData.converter_lock = torqueLocked;
    lock_color = (torqueLocked) ? 0xB7E0 : 0xFFFF;
    tft.setTextDatum(TL_DATUM);
    tft.fillRect(230, 185, 120, 80, TFT_BLACK);
    tft.setTextColor(lock_color, TFT_BLACK);
    if (isdigit(currentGear))
    {
      tft.setTextColor(lock_color, TFT_BLACK);
      String gear = String(currentGear);

      tft.drawString(gear, 255, 190, 7);
    }
    else
    {
      tft.drawChar(255, 190, currentGear, lock_color, TFT_BLACK, 6);
    }
    D_print("Current Gear: ");
    D_println(currentGear);
  }
}

String getDayOfTheWeak(DateTime dt, bool isShort = false)
{
  return isShort ? daysOfTheWeekShort[dt.dayOfTheWeek()] : daysOfTheWeek[dt.dayOfTheWeek()];
}

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
ICACHE_RAM_ATTR void setFlag(void)
{
  // we got a packet, set the flag
  receivedFlag = true;
}

void setupRTC()
{
  isRTC_Connected = rtc.begin();
  if (!isRTC_Connected)
  {

    D_println("RTC module is NOT found");
  }
}

void setupTPMS()
{
  D_println(F("Trying to setup TPMS"));
  tpmsSPI.begin(CC1101_SCK_PIN, CC1101_MISO_PIN, CC1101_MOSI_PIN, CC1101_CS_PIN);
  D_println(F("Setup TPMS"));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE)
  {
    isTPMS = true;
    D_println(F("success!"));
  }
  else
  {
    isTPMS = false;
    D_print(F("failed, code "));
    D_println(state);
    return;
  }
  D_print(F("[CC1101] Version "));
  D_println(radio.getChipVersion(), HEX);
  radio.setOOK(false);
  radio.setFrequency(433.92);
  radio.setRxBandwidth(162.0);
  radio.setBitRate(19.2);
  radio.setEncoding(RADIOLIB_ENCODING_NRZ);
  radio.setSyncWord(0x55, 0x56);
#define CHECKLENGTH 16
  radio.fixedPacketLengthMode(CHECKLENGTH);
  radio.setCrcFiltering(false);

  radio.setOutputPower(-10);
  radio.disableAddressFiltering();
  radio.setFrequencyDeviation(40.0);
  // set the function that will be called
  // when new packet is received
  radio.setPacketReceivedAction(setFlag);

  // start listening for packets
  D_print(F("[CC1101] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE)
  {
    D_println(F("success!"));
  }
  else
  {
    D_print(F("failed, code "));
    D_println(state);
    return;
  }
}



void lockDoor()
{
  if (isMcpOk)
  {
    isDoorLock = true;
    mcp.digitalWrite(BUTTON_LOCK_OUTPUT_PIN, true);
    lockButtonPressed = true;
    lockUnlockPressedMillis = millis();
  }
}

void unLockDoor()
{
  if (isMcpOk)
  {
    isDoorLock = false;
    isLockByAndriod = false;
    isLockByAndriodReboot = false;
    mcp.digitalWrite(BUTTON_UNLOCK_OUTPUT_PIN, true);
    unLockButtonPressed = true;
    lockUnlockPressedMillis = millis();
  }
}

void printKhudaHafiz()
{

  if (!isKhudaHafiz)
  {
    isKhudaHafiz = true;
    clearScreen();
    tft.drawString("kHUDA", 80, 80, 4);
    tft.drawString("HAFIZ", 130, 130, 4);
    // if (isDoorLock)
    //{
    unLockDoor();
    //}
  }
}

unsigned long prevInputPinMillis = 0;
void processInputPins(int delay)
{
  if (!isMcpOk)
  {
    return;
  }
  if (loopStartMillis - prevInputPinMillis > delay)
  {
    // D_println("processInputPins");
    prevInputPinMillis = loopStartMillis;
    isHeadLightOn = mcp.digitalRead(HEAD_LIGHT_PIN);
    isFL_DoorOpen = !mcp.digitalRead(DOOR_FL_PIN);
    isFR_DoorOpen = !mcp.digitalRead(DOOR_FR_PIN);
    isRL_DoorOpen = !mcp.digitalRead(DOOR_RL_PIN);
    isRR_DoorOpen = !mcp.digitalRead(DOOR_RR_PIN);
    isBack_DoorOpen = !mcp.digitalRead(DOOR_BACK_PIN);
    isAccOn = mcp.digitalRead(CAR_ACC_VOLTAGE_PIN);
    // D_print("isAccOn ");
    // D_print(isAccOn);
    // D_print("   prvIsAccOn ");
    // D_println(prvIsAccOn);
    if (prvIsAccOn != isAccOn)
    {
      prvIsAccOn = isAccOn;

      if (!isAccOn)
      {
        accOffTime = millis(); // Store the time ACC turned OFF
        printKhudaHafiz();
      }
      else
      {
        isKhudaHafiz = false;
        clearScreenReq = true;
        resetVariable();
        updateTPMSScreen = true;
        mcp.digitalWrite(ESP_LATCH_RELAY_OUTPUT_PIN, true);
      }
    }

    if (isFL_DoorOpen || isFR_DoorOpen || isRL_DoorOpen || isRR_DoorOpen || isBack_DoorOpen)
    {
      if (currentScreen != DOOR_OPEN_SCREEN)
      {
        previousScreen = currentScreen;
        currentScreen = DOOR_OPEN_SCREEN;
        resetVariable();
        isDoorLock = false;
        isLockByAndriod = false;
        isLockByAndriodReboot = false;
      }
      D_print(" DOOR OPENED => Old Screen: ");
      D_print(previousScreen);
      D_print(" New Screen: TPMS");
      D_println(currentScreen);
    }
    else if (currentScreen == DOOR_OPEN_SCREEN)
    {
      currentScreen = previousScreen;
      D_print(" DOOR CLOSED => Old Screen: TPMS ");
      D_print(previousScreen);
      D_print(" New Screen: ");
      D_println(currentScreen);
      // selectScreen(previousScreen);
      resetVariable();
      wasFL_DoorOpen = isFL_DoorOpen;
      wasFR_DoorOpen = isFR_DoorOpen;
      wasRL_DoorOpen = isRL_DoorOpen;
      wasRR_DoorOpen = isRR_DoorOpen;
      wasBack_DoorOpen = isBack_DoorOpen;
    }

    // Create a single string by concatenating the boolean values
    /*String output = String(isHeadLightOn ? "true" : "false") + "," +
                    String(isFL_DoorOpen ? "true" : "false") + "," +
                    String(isFR_DoorOpen ? "true" : "false") + "," +
                    String(isRL_DoorOpen ? "true" : "false") + "," +
                    String(isRR_DoorOpen ? "true" : "false") + "," +
                    String(isBack_DoorOpen ? "true" : "false");
    D_println(output);*/

    // D_print(" Head light Status: ");
    // D_println(isHeadLightOn);
  }
}
void processTPMS()
{
  if (!isTPMS)
  {
    return;
  }

  if (receivedFlag)
  {
    D_println("processTPMS");
    // reset flag
    receivedFlag = false;

    int i, k;
    String str;
    bool error;
    byte data[8];
    byte checksum;

    unsigned long tpms_id;
    float pressure;
    int temperature;
    // D_println("Redding TPMS Data ");
    //  you can read received data as an Arduino String
    int state = radio.readData(str);
    if (state == RADIOLIB_ERR_NONE && str.length() == CHECKLENGTH)
    {
      k = 0;
      error = false;
      for (i = 0; i < str.length(); i += 2)
      {
        data[k] = decode_manchester_byte(str.charAt(i), str.charAt(i + 1), error);
        k++;
      }
      if (!error)
      {
        checksum = 0;
        for (k = 0; k < 7; k++)
        {
          checksum = checksum + data[k];
        }
        if (checksum == data[7])
        {
          // https://github.com/merbanan/rtl_433/blob/master/src/devices/tpms_ford.c
          // ID, pressure decoding seems to work. Flags and temperature seem to be different for F2GT
          tpms_id = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
          pressure = data[4] * 0.25; //* 0.0689476; in psi
          temperature = -273;
          if (!(data[5] & 0x80))
          {
            temperature = data[5] - 56;
          }

          if (tpms_id != last_tpms_id || checksum != last_checksum)
          {
            if (tpms_id == ID_FL)
            {
              tyre_FL_PSI = pressure;
              tyre_FL_temp = (temperature != -273) ? temperature : 0;
              tyre_FL_time = millis();
              updateTPMSScreen = true;
            }
            else if (tpms_id == ID_FR)
            {
              tyre_FR_PSI = pressure;
              tyre_FR_temp = (temperature != -273) ? temperature : 0;
              tyre_FR_time = millis();
              updateTPMSScreen = true;
            }
            else if (tpms_id == ID_RL)
            {
              tyre_RL_PSI = pressure;
              tyre_RL_temp = (temperature != -273) ? temperature : 0;
              tyre_RL_time = millis();
              updateTPMSScreen = true;
            }
            else if (tpms_id == ID_RR)
            {
              tyre_RR_PSI = pressure;
              tyre_RR_temp = (temperature != -273) ? temperature : 0;
              tyre_RR_time = millis();
              updateTPMSScreen = true;
            }

            D_print("ID ");
            D_print(tpms_id, HEX);
            D_print(", ");
            D_print(pressure);
            D_print(" PSI, ");
            if (temperature != -273)
            {
              D_print(temperature);
            }
            else
            {
              D_print("?");
            }
            D_println(" °C ");

            last_tpms_id = tpms_id;
            last_checksum = checksum;
            isTyrePressureLow = (tyre_FL_PSI < lowPSI || tyre_RL_PSI < lowPSI || tyre_FR_PSI < lowPSI || tyre_RR_PSI < lowPSI);
          }
        }
      }
    }
    // put module back to listen mode
    radio.startReceive();
  }
}

void printDistance(float currentDistance)
{
  if (currentDistance != printedLcdData.trip_distance)
  {
    printedLcdData.trip_distance = currentDistance;
    float val = currentDistance;
    if (isnan(val))
    {
      val = 0;
    }
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    String formattedDistance = String(val, 1);
    while (formattedDistance.length() < 5)
    {
      formattedDistance = "0" + formattedDistance;
    }
    // tft.setFreeFont(FSB24);
    tft.drawString(formattedDistance, 170, 50, 4);
  }
}

void printTripAverage(float _currentTripAvg)
{
  if (_currentTripAvg > 99)
    return;
  float val = _currentTripAvg;
  if (isnan(val))
  {
    val = 0;
  }

  if (val != printedLcdData.trip_avg)
  {
    printedLcdData.trip_avg = val;
    String currentTripAvg = float2String(val);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(currentTripAvg, 170, 100, 4);
  }
}

void printTripFuel(float _currentTotalFuel)
{
  if (_currentTotalFuel > 99)
    return;
  float val = _currentTotalFuel;
  if (isnan(val))
  {
    val = 0;
  }

  if (val != printedLcdData.trip_fuel)
  {
    printedLcdData.trip_fuel = val;
    String currentFuel = float2String(val);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(currentFuel, 170, 145, 4);
  }
}

void calculateEngineRunningTime(int delay)
{
  static unsigned long _calculateEngineRunningTime = 0;
  if (millis() - _calculateEngineRunningTime > delay)
  {
    _calculateEngineRunningTime = millis();
  if (lcdData.inst_lph > 0)
  {
    isPetrolZero = false;
    if (!isEngineRunning)
    {
      isEngineRunning = true;
      engineStartTime = millis();
    }
  }
  else if (lcdData.inst_lph == 0 && isEngineRunning)
  {
    if (!isPetrolZero)
    {
      lastZeroTime = millis();
      isPetrolZero = true;
    }
    else if (millis() - lastZeroTime >= 10000)
    {
      // Engine has been stopped for more than 10 seconds
      isEngineRunning = false;
      // Calculate total engine running time
      previousEngineRunTime += millis() - engineStartTime;
    }
  }

  if (isEngineRunning)
  {
    currentEngineRunTime = millis() - engineStartTime + previousEngineRunTime;
  }
}
}

void printFuelPrice(float price)
{
  if (isnan(price))
  {
    price = 0;
  }
  if (price != printedLcdData.trip_price)
  {
    tft.setTextDatum(TL_DATUM);
    if (printedLcdData.trip_price != price)
    {
      tft.fillRect(5, 140, 100, 38, TFT_BLACK);
    }
    printedLcdData.trip_price = price;
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("$" + addCommas(price), 5, 145, 4);
  }
}
int instantAverageXpos = 50;
void printInstantAverage(float inst_kml, float inst_lph)
{
  float val = (inst_kml > 3) ? inst_kml : inst_lph;
  byte kpl_lh = (inst_kml > 3) ? 0 : 1;

  if (val > 99)
    val = 99;

  if (isnan(val))
  {
    val = 0;
  }
 // calculateEngineRunningTime(val);
  String currentInstantAvg = float2String(val);

  if (currentInstantAvg != previousInstantAvg)
  {
    previousInstantAvg = currentInstantAvg;
    int decimalIndex = currentInstantAvg.indexOf('.');
    String integerPart = currentInstantAvg.substring(0, decimalIndex + 1);
    String fractionalPart = currentInstantAvg.substring(decimalIndex + 1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    if (integerPart != previousInstantAvgInt)
    {
      tft.setFreeFont(&FreeSans36pt7b);
      instantAverageXpos = 50;
      instantAverageXpos += tft.drawString(integerPart, instantAverageXpos, 185, 1); // 195
      previousInstantAvgInt = integerPart;
    }

    tft.setTextFont(1);
    tft.drawString(fractionalPart, instantAverageXpos, 220, 4); // fractionalPart
    if (previous_kpl_lh != kpl_lh)
    {
      previous_kpl_lh = kpl_lh;
      if (kpl_lh == 0)
      {
        tft.drawString("Km/h", instantAverageXpos, 205, 2);
      }
      else
      {
        tft.drawString("L/h  ", instantAverageXpos, 205, 2);
      }
    }
  }
}

void printEngineRunningTime()
{
  String sVal = milli2Time(currentEngineRunTime);
  if (sVal != previousRunningTime)
  {
    previousRunningTime = sVal;
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);              // Centre text on x,y position
    tft.drawString(sVal, 80, 150, 4); // 65
  }
  //

  sVal = milli2Time(idleCarRunTimeMs);
  if (sVal != previousIdleRunningTime)
  {
    previousIdleRunningTime = sVal;
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);              // Centre text on x,y position
    tft.drawString(sVal, 245, 150, 4); // 65
  }
}
int pox = 0;
void printCurrentTime(DateTime dt)
{
  String timeString;
  uint8_t hour = (now.hour() % 12 ? now.hour() % 12 : 12);
  if (hour < 10)
    timeString += '0';
  timeString += hour;
  timeString += ':';
  if (now.minute() < 10)
    timeString += '0';
  timeString += now.minute();
  String SS;
  if (now.second() < 10)
    SS += '0';
  SS += now.second();
  String AM = now.hour() < 12 ? "am" : "pm";
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);

  if (PreviousTimeString != timeString)
  {
    PreviousTimeString = timeString;
    // tft.fillRect(2, 5, 100, 48, TFT_RED);
    pox = tft.drawString(timeString, 70, 55, 7);
  }
  tft.drawString(AM, (pox + 75), 55, 4);
  tft.drawString(SS, (pox + 75), 80, 4);

  String dateString = String(daysOfTheWeek[now.dayOfTheWeek()]) + ", " +
                      String(now.day()) + "  " +
                      String(monthsOfTheYear[now.month() - 1]) + "  " +
                      String(now.year());
  tft.setTextDatum(TC_DATUM);
  // D_println(dateString);
  tft.drawString(dateString, tft_W / 2, 110, 4);
}

float getRandomFloat(float min, float max)
{
  // Generate a random integer between 0 and 10000
  int randomInt = random(0, 10001);

  // Scale the random integer to the desired float range
  float randomFloat = min + (randomInt / 10000.0) * (max - min);

  return randomFloat;
}

void printSpeed(float speed)
{
  float val = speed;
  if (val > 220)
    val = 220;
  else if (val < 0)
    val = 0;
  else if (isnan(val))
    val = 0;
  if (val != printedLcdData.speed)
  {
    int oldDigits = (val < 10) ? 1 : (val < 100) ? 2
                                                 : 3;
    int newDigits = (printedLcdData.speed < 10) ? 1 : (printedLcdData.speed < 100) ? 2
                                                                                   : 3;
    if (oldDigits != newDigits)
    {
      tft.fillRect(20, 50, 300, 100, TFT_BLACK);
    }
    printedLcdData.speed = val;
    String currentSpeed = String(val, 2);
    int decimalIndex = currentSpeed.indexOf('.');
    String integerPart = currentSpeed.substring(0, decimalIndex + 1);
    String fractionalPart = currentSpeed.substring(decimalIndex + 1);
    D_println(integerPart);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(integerPart, 226, 65, 8);
    tft.drawString(fractionalPart, 280, 100, 6);
  }
}

void printRPM(uint16_t rpm, uint16_t avg_speed)
{
  uint16_t val = (rpm > 8000) ? 8000 : rpm;

  if (val != printedLcdData.rpm || avg_speed != printedLcdData.avg_speed)
  {
    if ((val < 1000 && printedLcdData.rpm >= 1000) ||
        (countDigits(avg_speed) != countDigits(printedLcdData.avg_speed)))
    {
      tft.fillRect(3, 145, 310, 35, TFT_BLACK);
    }
    printedLcdData.rpm = val;
    printedLcdData.avg_speed = avg_speed;
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    // tft.drawString(String(val), 175, 145, 4);
    tft.drawString(String(val), 80, 145, 4);
    tft.drawString(String(avg_speed), 240, 145, 4);
    tft.setTextDatum(TR_DATUM);
  }
}

void showCheckEnginePic()
{
  drawArrayJpeg(pic_check_engine, sizeof(pic_check_engine), 5, 5);
}

void showBatteryPic()
{
  drawArrayJpeg(pic_battery, sizeof(pic_battery), 60, 5);
}

void showOilLevelPic()
{
  drawArrayJpeg(pic_oil_level, sizeof(pic_oil_level), 60, 5);
}

void showAirBagPic()
{
  drawArrayJpeg(pic_air_bag, sizeof(pic_air_bag), 60, 5);
}

void showKeyPic()
{
  drawArrayJpeg(pic_key, sizeof(pic_key), 60, 5);
}

void showTPMSPic()
{
  drawArrayJpeg(pic_tpms, sizeof(pic_tpms), 60, 5);
}
/*
void showSeatBeltPic()
{
  drawArrayJpeg(pic_seat_belt, sizeof(pic_seat_belt), 60, 5);
}

void showFuelPic()
{
  drawArrayJpeg(pic_fuel, sizeof(pic_fuel), 5, 60);
}
*/
bool isCruiseShow = false;
void showCruiseActivePic()
{
  drawArrayJpeg(pic_cruise_active, sizeof(pic_cruise_active), 7, 200);
  isCruiseShow = true;
}
void showCruiseMainPic()
{
  drawArrayJpeg(pic_cruise_main, sizeof(pic_cruise_main), 7, 200);
  isCruiseShow = true;
}

// Array of images and related variables
PicDisplay picArray[] = {
    {showBatteryPic, &isBattery},
    {showOilLevelPic, &isOilPressure},
    {showAirBagPic, &isAirBag},
    {showKeyPic, &isImmobilizer},
    {showTPMSPic, &isTyrePressureLow}};

int currentErrorPicIndex = 0; // Track the current image index
void displayErrorPics()
{
  // Loop through the array to find the next valid image
  for (int i = 0; i < sizeof(picArray) / sizeof(picArray[0]); i++)
  {
    // Increment the index and wrap it around if necessary
    currentErrorPicIndex = (currentErrorPicIndex + 1) % (sizeof(picArray) / sizeof(picArray[0]));

    // Check if the associated condition is true
    if (*(picArray[currentErrorPicIndex].condition))
    {
      // Call the function to display the picture
      picArray[currentErrorPicIndex].func();
      break; // Show only one image at a time
    }
  }
}
// bool isHeaderClear = true;
void printCruiseIcon()
{
  static unsigned long lastHeaderCruisePrint = 0;
  if (isCruiseStart != prevCruiseStart || isCruiseMain != prevCruiseMain || (millis() - lastHeaderCruisePrint) > 99)
  {
    lastHeaderCruisePrint = millis();

    if (isCruiseStart)
    {
      showCruiseActivePic();
    }
    else if (isCruiseMain)
    {
      showCruiseMainPic();
    }
    else if (isCruiseShow)
    {
      tft.fillRect(5, 200, 40, 40, TFT_BLACK);
      isCruiseShow = false;
    }
    // Update previous states
    prevCruiseStart = isCruiseStart;
    prevCruiseMain = isCruiseMain;
  }
}

byte cnt = 1;
bool hasError = false;
void printHeader(int delay)
{
  static unsigned long lastHeaderPrint = 0;
  if (loopStartMillis - lastHeaderPrint > delay)
  {

    lastHeaderPrint = loopStartMillis;
    // isHeaderClear = true;
    //  D_println("printHeader");
    cnt++;
    spriteHeader.createSprite(320, 40); // Width: 320, Height: 40
    spriteHeader.fillSprite(TFT_BLACK); // Clear the sprite with black background
    drawVerticalGradientRectWithSplit(spriteHeader, 0, 0, 320, 40, TFT_MAROON, TFT_BLACK, 0.2);
    spriteHeader.setTextColor(TFT_WHITE);
    // tft.fillRectVGradient(0, 0, 320, 50, TFT_GREEN, TFT_YELLOW);
    // tft.setTextColor(TFT_WHITE, TFT_BLACK);
    spriteHeader.setTextDatum(TL_DATUM);
    // isCheckEngine = true;
    // isOilPressure = true;
    //  isBattery = true;
    //  isAirBag = true;
    // isTyrePressureLow = false;
    if (isTyrePressureLow || isCheckEngine || isBattery || isOilPressure || isAirBag || isImmobilizer)
    {
      hasError = true;
    }
    else
    {
      hasError = false;
    }

    if (!hasError)
    {
      int xPos = spriteHeader.drawString(String(carTemperature, 1) + "c", 10, 10, 4);
      spriteHeader.drawString(String(lcdData.outSideTemp, 1) + "c", (xPos + 20), 15, 2);
      // xPos = tft.drawString("c", (xPos + 20), 10, 4);
      // tft.drawCircle((xPos + 40), 15, 3, TFT_WHITE); // Adjust position and size as needed
      /*if (humidity > 0 && humidity < 100)
      {
        spriteHeader.drawString(String(humidity, 0) + "%", (xPos + 20), 15, 2);
      }*/
    }
    if ((cnt / 5) % 2 == 0)
    {
      spriteHeader.setTextDatum(TL_DATUM);
      // lastValidFuelLevel = 15;
      spriteHeader.drawString(String((lastValidFuelLevel * 5)) + "%", 150, 10, 4);
    }
    else
    {
      spriteHeader.setTextDatum(TL_DATUM);
      // OBD2Data.EngineTemp1 = 35;
      if (!isnan(OBD2Data.EngineTemp1))
      {
        spriteHeader.drawString(String(OBD2Data.EngineTemp1, 0) + "c", 150, 10, 4);
      }
      else
      {
        spriteHeader.drawString("?c", 150, 10, 4);
      }
    }

    spriteHeader.setTextDatum(TR_DATUM);
    String timeString;
    uint8_t hour = (now.hour() % 12 ? now.hour() % 12 : 12);
    if (hour < 10)
      timeString += '0';
    timeString += hour;
    timeString += ':';
    if (now.minute() < 10)
      timeString += '0';
    timeString += now.minute();

    String AM = now.hour() < 12 ? "AM" : "PM";

    spriteHeader.drawString(timeString, (tft_W - 30), 10, 4);
    spriteHeader.drawString(AM, tft_W - 10, 15, 2);
    // spriteHeader.drawFastHLine(1, 40, 320, TFT_WHITE); // 1st Line
    // tft.setTextDatum(TL_DATUM);
    //  Push the sprite to the screen
    spriteHeader.pushSprite(0, 0);            // Push sprite to position (x=0, y=0)
    spriteHeader.deleteSprite();              // Free sprite memory
    tft.drawFastHLine(1, 40, 320, TFT_WHITE); // 1st Line
    if (hasError)
    {
      if (isCheckEngine)
      {
        showCheckEnginePic();
      }

      // if (cnt % 2 == 0)
      //{
      displayErrorPics(); // Call the function when `cnt` is even
      //}
    }
  }
}

void printScreenGrid(Screen screen)
{
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (screen == TRIP_SCREEN)
  {
    tft.drawString("Distance", 5, 50, 4);
    tft.drawString("km", 245, 50, 4);
    tft.drawString("Trip " + String(currentTrip + 1), 5, 100, 4);
    tft.drawString("km/h", 245, 100, 4);
    tft.drawString("Ltr", 245, 145, 4);

    tft.drawFastHLine(1, 86, tft_W, TFT_WHITE);  // 1st Line
    tft.drawFastHLine(1, 132, tft_W, TFT_WHITE); // 2nd Line
    tft.drawFastVLine(120, 40, 140, TFT_WHITE);  // 1st 2nd Title Line
  }
  else if (screen == INJECTOR_TIMING_SCREEN)
  {
    tft.drawString("Inj Time", 5, 50, 4);
    tft.drawString("us", 245, 50, 4);

    tft.drawString("Inj Duty", 5, 100, 4);
    tft.drawString("%", 245, 100, 4);

    // tft.drawString("Inj Freq", 5, 145, 4);
    tft.drawString("Freq", 245, 145, 4);

    tft.drawFastHLine(1, 86, tft_W, TFT_WHITE);  // 1st Line
    tft.drawFastHLine(1, 132, tft_W, TFT_WHITE); // 2nd Line
    tft.drawFastVLine(120, 40, 140, TFT_WHITE);  // 1st 2nd Title Line
  }
  else if (screen == CURRENT_TIME_SCREEN)
  {
    tft.drawFastHLine(1, 140, tft_W, TFT_WHITE); // 2nd Line
  }
  tft.drawFastHLine(1, 180, tft_W, TFT_WHITE); // 3rd Line
  tft.drawFastVLine(220, 180, 160, TFT_WHITE); // gear  Line
}

long lastSendMillis = 0;
void send_page_number(int tripNo, byte buttonState)
{
  if (millis() - lastSendMillis > 100)
  {
    lastSendMillis = millis();
    sendBackData.tripNo = tripNo;
    sendBackData.buttonState = buttonState;
    preferences.begin("zApp", false);
    preferences.putInt("currentTrip", tripNo);
    preferences.end();
    esp_err_t result = ESP_OK;
    result = esp_now_send(broadcastAddress, (uint8_t *)&sendBackData, sizeof(sendBackData));
    if (result == ESP_OK)
    {
      D_println("Sent with success");
    }
    else
    {
      D_print("Error sending the data ");
      D_println(esp_err_to_name(result));
    }
  }
}

void showCarPic()
{
  drawArrayJpeg(pic_car, sizeof(pic_car), 80, 50);
}

void showCarWithTyrePic()
{
  drawArrayJpeg(pic_car_with_tyre, sizeof(pic_car_with_tyre), 80, 50);
}

void showDoorFLPic()
{
  drawArrayJpeg(pic_door_fl, sizeof(pic_door_fl), 113, 82);
}

void showDoorFRPic()
{
  drawArrayJpeg(pic_door_fr, sizeof(pic_door_fr), 185, 82);
}

void showDoorRLPic()
{
  drawArrayJpeg(pic_door_rl, sizeof(pic_door_rl), 113, 110);
}

void showDoorRRPic()
{
  drawArrayJpeg(pic_door_rr, sizeof(pic_door_rr), 185, 110);
}

void showDoorBackPic()
{
  drawArrayJpeg(pic_door_back, sizeof(pic_door_back), 148, 145);
}

const int freq = 5000;    // PWM frequency in Hz
const int ledChannel = 1; // PWM channel
const int resolution = 8; // Resolution (8 bits = 0-255)

// Function to initialize the PWM
void setupPWM()
{
  ledcSetup(ledChannel, freq, resolution);  // Configure PWM settings
  ledcAttachPin(LCD_LIGHT_PIN, ledChannel); // Attach PWM to pin
  ledcWrite(ledChannel, 0);
}

// Custom function to mimic analogWrite
void customAnalogWrite(int pin, int value)
{
  // Ensure the value is within the resolution range
  value = constrain(value, 0, 255);
  if (pin == LCD_LIGHT_PIN)
  {
    ledcWrite(ledChannel, value); // Write PWM duty cycle
  }
}

#define VREF 3.3 // Reference voltage for DAC (usually 3.3V)
void outputVoltageDAC(float voltage)
{
  // Limit voltage to 0 - VREF range
  voltage = constrain(voltage, 0.0, VREF);
  // Convert voltage to 8-bit DAC value
  uint8_t dacValue = (voltage / VREF) * 255.0;
  // Output to DAC
  dacWrite(AC_SUN_SENSOR_PIN, dacValue);
}

void setScreenDim(bool isLightOn)
{

  if (isLightOn)
  {
    customAnalogWrite(LCD_LIGHT_PIN, step3);
    // digitalWrite(LCD_LIGHT_PIN, 1);
    isScreenDim = true;
    D_println("Screen Dim");
  }
  else
  {
    customAnalogWrite(LCD_LIGHT_PIN, 255);
    // digitalWrite(LCD_LIGHT_PIN, 1);
    isScreenDim = false;
    D_println("Screen fullbright");
  }
}

void SetupESP_NOW()
{
  lcdDataSize = sizeof(lcdData);
  doorDataSize = sizeof(doorData);
  WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    D_println("Error initializing ESP-NOW");
  }

  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = WiFi.channel();
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    D_println("Failed to add peer");
  }

  /*
    int chan;
    if (int32_t n = WiFi.scanNetworks())
    {
      for (uint8_t i = 0; i < n; i++)
      {
        if (!strcmp(ssid, WiFi.SSID(i).c_str()))
        {
          chan = WiFi.channel(i);
        }
      }
    }

    wifi_set_channel(chan);
    */
}

void saveIDs()
{
  preferences.begin("zApp", false);
  preferences.putULong("id_fl", ID_FL);
  preferences.putULong("id_fr", ID_FR);
  preferences.putULong("id_rl", ID_RL);
  preferences.putULong("id_rr", ID_RR);
  preferences.end();
}

void startServer()
{
  const char *ssid = "Reborn-LCD";
  const char *password = "Z.12345678";

  IPAddress apIP(192, 168, 4, 1);
  // WiFi.mode(WIFI_AP);
  WiFi.mode(WIFI_AP_STA);
  D_print("MAC Address");
  D_println(WiFi.macAddress());
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);

  // DNS server for captive portal
  dnsServer.start(53, "*", apIP);

  /*
  const char *zSsid = "Logout";
  const char *zPassword = "ms120400427";
  WiFi.begin(zSsid, zPassword);

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }


  // Connected
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
*/
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", (isOBDServer ? P_OBD_MAIN_PAGE : P_MAIN_PAGE), nullptr); });

  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->redirect("/"); });
  // Handle form submission
  server.on("/setTime", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("date", true) && request->hasParam("time", true)) {
      String date = request->getParam("date", true)->value();
      String time = request->getParam("time", true)->value();

      // Parse date and time
      DateTime newTime = DateTime(date.substring(0, 4).toInt(), date.substring(5, 7).toInt(), date.substring(8, 10).toInt(),
                                  time.substring(0, 2).toInt(), time.substring(3, 5).toInt(), time.substring(6, 8).toInt());

      // Adjust RTC time
      if(isRTC_Connected){
       rtc.adjust(newTime);
      request->send(200, "text/plain", "Time updated successfully!");
      } else{
        request->send(200, "text/plain", "RTC not connected!");
      }
      
    } else {
      request->send(400, "text/plain", "Invalid input. Please provide both date and time.");
    } });

  server.on("/get_ids", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String json = "{";
    json += "\"id_fl\":" + String(ID_FL) + ",";
    json += "\"id_fr\":" + String(ID_FR) + ",";
    json += "\"id_rl\":" + String(ID_RL) + ",";
    json += "\"id_rr\":" + String(ID_RR);
    json += "}";
    D_println(json);
    request->send(200, "application/json", json); });

  server.on("/set_ids", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
      String body = "";
      for (size_t i = 0; i < len; i++) body += (char)data[i];
      
      //StaticJsonDocument<200> doc;
      JsonDocument doc;
          DeserializationError error = deserializeJson(doc, body);
      if (!error) {
        if (doc.containsKey("id_fl")) ID_FL = strtoul(doc["id_fl"], NULL, 16);
        if (doc.containsKey("id_fr"))
          ID_FR = strtoul(doc["id_fr"], NULL, 16);
        if (doc.containsKey("id_rl"))
          ID_RL = strtoul(doc["id_rl"], NULL, 16);
        if (doc.containsKey("id_rr")) ID_RR = strtoul(doc["id_rr"], NULL, 16);
        saveIDs();
        request->send(200, "text/plain", "OK");
      } else {
        request->send(400, "text/plain", "Invalid JSON");
      } });

  server.on("/update_default", HTTP_GET, [](AsyncWebServerRequest *request)
            {
          preferences.begin("zApp", false);
          preferences.putULong("id_fl", ID_FL_DEF);
          preferences.putULong("id_fr", ID_FR_DEF);
           preferences.putULong("id_rl", ID_RL_DEF);
          preferences.putULong("id_rr", ID_RR_DEF);
          preferences.end();

          ID_FL = ID_FL_DEF;
          ID_FR = ID_FR_DEF;
          ID_RL = ID_RL_DEF;
          ID_RR = ID_RR_DEF;
        
      D_println("update_default");
      request->send(200, "text/html", "OK"); });

  server.on("/lock", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              isLockByAndriod = true;     
              lastLockByAndriodTime = millis();
              D_println("lock Door");
             lockDoor();
      request->send(200, "text/html", "OK"); });

  server.on("/unlock", HTTP_GET, [](AsyncWebServerRequest *request)
            {       
              unLockDoor();
      D_println("unlock Door");
      request->send(200, "text/html", "OK"); });



  server.begin();
  D_println("Web server running...");
}

void setupTelnetServer()
{
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  D_println("telnet server running...");
}

void handleTelnet()
{
  if (telnetServer.hasClient())
  {
    if (telnetClient && telnetClient.connected())
      telnetClient.stop();

    telnetClient = telnetServer.available();
    telnetClient.print("Telnet Client Connected\r\n");
  }

  if (!telnetClient || !telnetClient.connected())
    return;

  static char cmd_buffer[64];
  static uint8_t index = 0;

  while (telnetClient && telnetClient.connected() && telnetClient.available())
  {
    char c = telnetClient.read();

    if (c == '\n' || c == '\r')
    {

      cmd_buffer[index] = '\0';
      if (index < 3)
      {
        index = 0;
        return;
      }
      // remove trailing spaces and hidden chars
      for (int i = 0; i < index; i++)
      {
        if (cmd_buffer[i] < 32 || cmd_buffer[i] > 126)
        {
          cmd_buffer[i] = '\0';
          break;
        }
      }
      index = 0;
      if (strcmp(cmd_buffer, "reboot") == 0)
      {
        telnetClient.println("Rebooting...");
        delay(1000);
        ESP.restart();
      }
      else if (strcmp(cmd_buffer, "lock") == 0)
      {
        isLockByAndriod = true;
        lastLockByAndriodTime = millis();
        lockDoor();
        telnetClient.println("STATUS: lock");
      }
      else if (strcmp(cmd_buffer, "unlock") == 0)
      {
        unLockDoor();
        telnetClient.println("STATUS: unlock");
      }
      else if (strcmp(cmd_buffer, "heap") == 0)
      {
        uint32_t free_heap = ESP.getFreeHeap();
        uint32_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
        uint32_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        uint32_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

        float frag_index = 1.0 - ((float)largest_block / free_heap);

        telnetClient.println("\n==== ESP32 HEAP STATUS ====");

        telnetClient.printf("Free Heap        : %u bytes\r\n", free_heap);
        telnetClient.printf("Largest Block    : %u bytes\r\n", largest_block);
        telnetClient.printf("Internal Free    : %u bytes\r\n", internal_free);
        telnetClient.printf("PSRAM Free       : %u bytes\r\n", psram_free);

        telnetClient.printf("Fragmentation    : %.2f\r\n", frag_index);

        telnetClient.println("----------------------------");

        if (largest_block < 8000)
          telnetClient.println("STATUS: WARNING");
        else if (largest_block < 20000)
          telnetClient.println("STATUS: OK");
        else
          telnetClient.println("STATUS: GOOD");

        telnetClient.println("============================\r\n");
      }

      else if (strcmp(cmd_buffer, "help") == 0 || strcmp(cmd_buffer, "?") == 0)
      {
        telnetClient.println("\r\n==== COMMANDS ====");
        telnetClient.println("heap");
        telnetClient.println("reboot");
        telnetClient.println("lock");
        telnetClient.println("unlock");
        telnetClient.println("==================\r\n");
      }
      else
      {
        telnetClient.print("Unknown command: ");
        telnetClient.println(cmd_buffer);
      }
    }
    else
    {
      if (index < sizeof(cmd_buffer) - 1)
        cmd_buffer[index++] = c;
      else
        index = 0; // overflow protection
    }
  }
}

void click(Button2 &btn)
{
  if (currentScreen != NATIVE_BUTTON_SCREEN)
  {
    bool isPlus = (btn == btnPlus);

    if (isPlus)
    {
      // Check if current screen handles internal sub-pages (e.g., Trips)
      if (screenList[currentScreenIndex].hasSubPages && currentTrip < totalTrips)
      {
        currentTrip++;
        send_page_number(currentTrip, 1);
      }
      else
      {
        // Move to next screen in the array (wraps around automatically)
        currentScreenIndex = (currentScreenIndex + 1) % NUM_CONFIGURED_SCREENS;
        currentScreen = screenList[currentScreenIndex].screenId;

        // Execute handler if specified
        if (screenList[currentScreenIndex].onEnterForward) {
          screenList[currentScreenIndex].onEnterForward();
        }
      }
    }
    else // btnMinus
    {
      // Check if current screen handles internal sub-pages (e.g., Trips)
      if (screenList[currentScreenIndex].hasSubPages && currentTrip > 0)
      {
        currentTrip--;
        send_page_number(currentTrip, 1);
      }
      else
      {
        // Move to previous screen in the array (wraps around automatically)
        currentScreenIndex = (currentScreenIndex - 1 + NUM_CONFIGURED_SCREENS) % NUM_CONFIGURED_SCREENS;
        currentScreen = screenList[currentScreenIndex].screenId;

        // Execute handler if specified
        if (screenList[currentScreenIndex].onEnterBackward) {
          screenList[currentScreenIndex].onEnterBackward();
        }
      }
    }

    // Save state to NVS preferences
    preferences.begin("zApp", false);
    preferences.putInt("currentScreen", static_cast<int>(currentScreen));
    preferences.putInt("lcdPage", currentScreen);
    preferences.end();
    
    resetVariable();

    // Debug output
    D_print(isPlus ? "btnPlus pressed " : "btnMinus pressed ");
    D_print(" => Current Screen: ");
    D_print(screenList[currentScreenIndex].name);
    D_print("  Trip #: ");
    D_println((currentTrip + 1));
  }
}

/*
void click_old(Button2 &btn)
{
  if (currentScreen != NATIVE_BUTTON_SCREEN)
  {
    // Update lcdPage based on button press

    // currentScreen = (btn == btnPlus) ? static_cast<Screen>((currentScreen + 1) % TOTAL_SCREENS) : //static_cast<Screen>((currentScreen - 1 + TOTAL_SCREENS) % TOTAL_SCREENS);

    if (btn == btnPlus)
    {
      switch (currentScreen)
      {
      case CURRENT_TIME_SCREEN:
        currentScreen = TPMS_SCREEN;
        updateTPMSScreen = true;
        break;

      case TPMS_SCREEN:
        currentScreen = SPEED_SCREEN;
        break;

      case SPEED_SCREEN:
        currentScreen = INJECTOR_TIMING_SCREEN;
        break;

      case INJECTOR_TIMING_SCREEN:
        currentScreen = TRIP_SCREEN;
        currentTrip = 0;
        send_page_number(currentTrip, 1);
        break;

      case TRIP_SCREEN:
        if (currentTrip < totalTrips)
        {
          currentTrip++;
          send_page_number(currentTrip, 1);
        }
        else
        {
          currentScreen = CURRENT_TIME_SCREEN;
        }
        break;

      default:
        currentScreen = CURRENT_TIME_SCREEN;
        break;
      }
    }
    else if (btn == btnMinus)
    {
      switch (currentScreen)
      {
      case TRIP_SCREEN:
        if (currentTrip > 0)
        {
          currentTrip--;
          send_page_number(currentTrip, 1);
        }
        else
        {
          currentScreen = INJECTOR_TIMING_SCREEN;
        }
        break;

      case INJECTOR_TIMING_SCREEN:
        currentScreen = SPEED_SCREEN;
        break;

      case SPEED_SCREEN:
        currentScreen = TPMS_SCREEN;
        updateTPMSScreen = true;
        break;

      case TPMS_SCREEN:
        currentScreen = CURRENT_TIME_SCREEN;
        break;

      case CURRENT_TIME_SCREEN:
        if (totalTrips > 0)
        {
          currentScreen = TRIP_SCREEN;
          currentTrip = totalTrips;
          clearScreen();
          send_page_number(currentTrip, 1);
          clearScreenReq = true;
        }
        break;

      default:
        break;
      }
    }
    // Save the current page to preferences
    preferences.begin("zApp", false);
    preferences.putInt("currentScreen", static_cast<int>(currentScreen));
    preferences.putInt("lcdPage", currentScreen);
    preferences.end();
    resetVariable();
    // Debug output
    D_print((btn == btnPlus) ? "btnPlus pressed " : "btnMinus pressed ");
    D_print(" => Current Screen: ");
    D_print(screenNames[currentScreen]);
    D_print("  Trip #: ");
    D_println((currentTrip + 1));
  }
}
*/

void longClickDetected(Button2 &btn)
{
  if (currentScreen != NATIVE_BUTTON_SCREEN)
  {
    if (btn == btnPlus)
    {
      D_println("btnPlus Long click");
      clearScreen();
      initPrvLcdData();
      resetVariable();
      selectScreen(CURRENT_TIME_SCREEN);
      temporaryScreen = true;
      temporaryScreenMillis = millis();
    }
    else
    {
      if (currentScreen == TRIP_SCREEN)
        send_page_number(currentTrip, 8); // send minus button long click event to esp32
    }
  }
}

void doubleClick(Button2 &btn)
{
}

void tripleClick(Button2 &btn)
{
  // btn.getNumberOfClicks();
}

void pressed(Button2 &btn)
{
  if (currentScreen == NATIVE_BUTTON_SCREEN)
  {
    temporaryScreenMillis = millis();
    nativeButtonLongClick = true;
    if (isMcpOk)
    {
      if (btn == btnPlus)
        mcp.digitalWrite(BUTTON_PLUS_OUTPUT_PIN, HIGH);
      else
        mcp.digitalWrite(BUTTON_MINUS_OUTPUT_PIN, HIGH);
    }
    D_println("Native Button Click");
  }
  else
  {
    btnCounter++;
    if (btnCounter >= 2)
    {
      btnCounter = 2;
      clearScreen();
      tft.drawString("Native", 90, 100, 4);
      tft.drawString("Buttons", 120, 130, 4);
      tft.drawString("Mode", 150, 160, 4);
      tft.drawRoundRect(60, 60, 200, 160, 5, TFT_GREEN);
      temporaryScreenMillis = millis();
      selectScreen(NATIVE_BUTTON_SCREEN);
    }
  }
}

void released(Button2 &btn)
{
  if (currentScreen == NATIVE_BUTTON_SCREEN)
  {
    nativeButtonLongClick = false;
    temporaryScreenMillis = millis();
    if (isMcpOk)
    {
      if (btn == btnPlus)
        mcp.digitalWrite(BUTTON_PLUS_OUTPUT_PIN, LOW);
      else
        mcp.digitalWrite(BUTTON_MINUS_OUTPUT_PIN, LOW);
    }

    D_println("Native Button Release");
  }
  else
  {
    btnCounter--;
  }
}

void setupButtons()
{
  btnPlus.begin(BUTTON_PLUS_PIN);
  btnPlus.setLongClickTime(300);
  btnPlus.setDoubleClickTime(300);
  btnPlus.setClickHandler(click);
  btnPlus.setLongClickDetectedHandler(longClickDetected);
  btnPlus.setLongClickDetectedRetriggerable(false);
  // btnPlus.setDoubleClickHandler(doubleClick);
  // btnPlus.setTripleClickHandler(tripleClick);
  btnPlus.setPressedHandler(pressed);
  btnPlus.setReleasedHandler(released);

  btnMinus.begin(BUTTON_MINUS_PIN);
  btnMinus.setLongClickTime(300);
  btnMinus.setDoubleClickTime(600);
  btnMinus.setClickHandler(click);
  btnMinus.setLongClickDetectedHandler(longClickDetected);
  btnMinus.setLongClickDetectedRetriggerable(false);
  // btnMinus.setDoubleClickHandler(doubleClick);
  // btnMinus.setTripleClickHandler(tripleClick);
  btnMinus.setPressedHandler(pressed);
  btnMinus.setReleasedHandler(released);
}

void print_trip_screen()
{

  printDistance(lcdData.trip_distance);
  printTripAverage(lcdData.trip_avg);
  printTripFuel(lcdData.trip_fuel);
  printFuelPrice(lcdData.trip_price);
  printInstantAverage(lcdData.inst_kml, lcdData.inst_lph);
  printGear(lcdData.gear, lcdData.converter_lock);
  printScreenGrid(currentScreen);
}

void print_time_screen()
{
  if (!updateScreen)
    return;

  printCurrentTime(now);
  printEngineRunningTime();
  printInstantAverage(lcdData.inst_kml, lcdData.inst_lph);
  printGear(lcdData.gear, lcdData.converter_lock);
  printScreenGrid(currentScreen);
}

void print_Speed_screen()
{
  if (!updateScreen)
    return;
  printSpeed(lcdData.speed);
  printRPM(lcdData.rpm, avg_speed);
  printInstantAverage(lcdData.inst_kml, lcdData.inst_lph);
  printGear(lcdData.gear, lcdData.converter_lock);
  printScreenGrid(currentScreen);
}

void print_Injector_Timing_screen()
{
  if (!updateScreen)
    return;

  // Print Injector Timing in ms
  if (lcdData.inj_timing != printedLcdData.inj_timing)
  {
    if ((lcdData.inj_timing < 9999 && printedLcdData.inj_timing >= 9999) ||
        lcdData.inj_timing == 0)
    {
      tft.fillRect(170, 50, 75, 35, TFT_BLACK);
    }

    printedLcdData.inj_timing = lcdData.inj_timing;
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    if (printedLcdData.inj_timing > 0)
      tft.drawString(String(printedLcdData.inj_timing), 170, 50, 4);
    else
      tft.drawString("0000", 170, 50, 4);
  }

  float val = 0;
  if (!isnanf(lcdData.duty_cycle))
  {
    val = lcdData.duty_cycle;
  }
  // Print Injector Duty Cycle in percentage
  if (val != printedLcdData.duty_cycle)
  {
    printedLcdData.duty_cycle = val;
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(float2String(printedLcdData.duty_cycle), 170, 100, 4);
  }

  val = 0;
  if (!isnanf(lcdData.inj_freq))
  {
    val = lcdData.inj_freq;
  }
  // Print Injector Frequency in Hz
  if (val != printedLcdData.inj_freq)
  {
    if (val < 100 && printedLcdData.inj_freq >= 100)
    {
      tft.fillRect(170, 145, 75, 35, TFT_BLACK);
    }

    printedLcdData.inj_freq = val;
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(float2String(printedLcdData.inj_freq), 170, 145, 4);
  }

  val = 0;
  if (!isnanf(lcdData.inst_lph))
  {
    val = lcdData.inst_lph;
  }

  // Print Instantaneous Ltr per Hour
  if (val != printedLcdData.inst_lph)
  {
    tft.setTextDatum(TL_DATUM);
    printedLcdData.inst_lph = val;
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(float2String(printedLcdData.inst_lph) + " L/h", 5, 145, 4);
  }

  printInstantAverage(lcdData.inst_kml, lcdData.inst_lph);
  printGear(lcdData.gear, lcdData.converter_lock);
  printScreenGrid(currentScreen);
}

unsigned int text_PSI_color = 0x7BEF;
void print_TPMS_screen()
{
  printInstantAverage(lcdData.inst_kml, lcdData.inst_lph);
  printGear(lcdData.gear, lcdData.converter_lock);
  printScreenGrid(currentScreen);

  if (!updateTPMSScreen)
    return;
  updateTPMSScreen = false;
  tft.setTextDatum(TL_DATUM);
  showCarWithTyrePic();
  // FL
  tft.setTextDatum(TR_DATUM);
  text_PSI_color = (tyre_FL_PSI < lowPSI || (millis() - tyre_FL_time) > 600000) ? 0xF800 : 0xFFFF;

  tft.setTextColor(text_PSI_color, TFT_BLACK);
  // tyre_FL_PSI = 30.00;
  // tyre_FL_temp = 34;
  int pos = tft.drawString(getFractionalPart(tyre_FL_PSI), 125, 75, 2);
  tft.drawString(getIntegerPart(tyre_FL_PSI), (125 - pos), 70, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(tyre_FL_temp) + "c", 125, 50, 2);

  // RL
  text_PSI_color = (tyre_RL_PSI < lowPSI || (millis() - tyre_RL_time) > 600000) ? 0xF800 : 0xFFFF;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // tyre_RL_PSI = 54.25;
  // tyre_RL_temp = 25;
  tft.drawString(String(tyre_RL_temp) + "c", 125, 155, 2);
  tft.setTextColor(text_PSI_color, TFT_BLACK);
  pos = tft.drawString(getFractionalPart(tyre_RL_PSI), 125, 135, 2);
  tft.drawString(getIntegerPart(tyre_RL_PSI), (125 - pos), 130, 4);

  // FR
  tft.setTextDatum(TL_DATUM);
  text_PSI_color = (tyre_FR_PSI < lowPSI || (millis() - tyre_FR_time) > 600000) ? 0xF800 : 0xFFFF;
  // tyre_FR_PSI = 17.50;
  // tyre_FR_temp = 35;
  tft.setTextColor(text_PSI_color, TFT_BLACK);
  pos = tft.drawString(getIntegerPart(tyre_FR_PSI), 200, 70, 4);
  tft.drawString(getFractionalPart(tyre_FR_PSI), (pos + 200), 75, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(tyre_FR_temp) + "c", 200, 50, 2);

  // RR
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // tyre_RR_PSI = 18.75;
  // tyre_RR_temp = 15;
  tft.drawString(String(tyre_RR_temp) + "c", 200, 155, 2);
  text_PSI_color = (tyre_RR_PSI < lowPSI || (millis() - tyre_RR_time) > 600000) ? 0xF800 : 0xFFFF;
  tft.setTextColor(text_PSI_color, TFT_BLACK);
  pos = tft.drawString(getIntegerPart(tyre_RR_PSI), 200, 130, 4);
  tft.drawString(getFractionalPart(tyre_RR_PSI), (pos + 200), 135, 2);
}
/*
void print_TPMS_screen1()
{
  showCarWithTyrePic();
  // FL
  int pos = tft.drawString("30", 5 + headerHeight, 20, 4);
  tft.drawString(".2", (pos + 5 + headerHeight), 25, 2);
  tft.drawString("26c", 25 + headerHeight, 40, 2);

  // RL
  pos = tft.drawString("32", 5 + headerHeight, 85, 4);
  tft.drawString(".8", (pos + headerHeight + 5), 90, 2);
  tft.drawString("25c", 25 + headerHeight, 105, 2);

  // RL
  pos = tft.drawString("29", 120, 20, 4);
  tft.drawString(".8", (pos + 120), 25, 2);
  tft.drawString("26c", 120, 40, 2);

  // RR
  pos = tft.drawString("28", 120, 85, 4);
  tft.drawString(".3", (pos + 120), 90, 2);
  tft.drawString("25c", 120, 105, 2);
}
*/
void setupMcp()
{
  isMcpOk = mcp.begin_I2C(0x27);
  if (!isMcpOk)
  {
    D_println("MCP Not Work ");
    return;
  }
}

void setupPin()
{
  D_println("setupPin");
  pinMode(BUTTON_PLUS_PIN, INPUT);
  pinMode(BUTTON_MINUS_PIN, INPUT);

  if (isMcpOk)
  {
    D_println("Setup MCP Pin");
    mcp.pinMode(DOOR_FL_PIN, INPUT);
    mcp.pinMode(DOOR_FR_PIN, INPUT);
    mcp.pinMode(DOOR_RL_PIN, INPUT);
    mcp.pinMode(DOOR_RR_PIN, INPUT);
    mcp.pinMode(DOOR_BACK_PIN, INPUT);
    mcp.pinMode(HEAD_LIGHT_PIN, INPUT);
    mcp.pinMode(CAR_ACC_VOLTAGE_PIN, INPUT);

    mcp.pinMode(BUTTON_PLUS_OUTPUT_PIN, OUTPUT);
    mcp.pinMode(BUTTON_MINUS_OUTPUT_PIN, OUTPUT);
    mcp.pinMode(BUTTON_LOCK_OUTPUT_PIN, OUTPUT);
    mcp.pinMode(BUTTON_UNLOCK_OUTPUT_PIN, OUTPUT);
    mcp.pinMode(ESP_LATCH_RELAY_OUTPUT_PIN, OUTPUT);

    mcp.digitalWrite(BUTTON_PLUS_OUTPUT_PIN, false);
    mcp.digitalWrite(BUTTON_MINUS_OUTPUT_PIN, false);
    mcp.digitalWrite(BUTTON_LOCK_OUTPUT_PIN, false);
    mcp.digitalWrite(BUTTON_UNLOCK_OUTPUT_PIN, false);
    mcp.digitalWrite(ESP_LATCH_RELAY_OUTPUT_PIN, false);
  }
}

void setupScreen()
{
  D_println("Setup Screen");
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);
  tft_W = tft.width();
  tft_H = tft.height();
  initPrvLcdData();
}
void getMeterData(int delay);

void printPray()
{
  D_println("printPray");
  // tft.setRotation(2); // portrait
  clearScreen();
  drawArrayJpeg(pic_dua, sizeof(pic_dua), 0, 45); // Draw a jpeg image stored in memory
  setScreenDim(false);
  unsigned long startTime = millis();
  bool waitForComplete = true;
  int cnt = 0;
  while (waitForComplete)
  {
    loopStartMillis = millis();
    getMeterData(100);
    unsigned long previousMillis = 0; // Stores the last time the event was executed
    const long interval = 100;

    if (loopStartMillis - previousMillis >= interval)
    {
      previousMillis = loopStartMillis; // Update the time of the last execution

      cnt++;
      if (millis() - startTime >= 3000)
      {
        waitForComplete = false;
      }

      if (isCheckEngine)
      {
        showCheckEnginePic();
      }
      if (isBattery)
      {
        drawArrayJpeg(pic_battery, sizeof(pic_battery), 80, 5);
      }
      if (isOilPressure)
      {
        drawArrayJpeg(pic_oil_level, sizeof(pic_oil_level), 140, 5);
      }
      if (isImmobilizer)
      {
        drawArrayJpeg(pic_key, sizeof(pic_key), 230, 5);
      }
      if (isAirBag)
      {
        drawArrayJpeg(pic_air_bag, sizeof(pic_air_bag), 280, 55);
      }
      int buttonState = digitalRead(BUTTON_PLUS_PIN);
      if (buttonState == LOW)
      {
        isSpecialMode = true;
      }

      if (isSpecialMode)
      {
        tft.setTextColor(TFT_GREEN);
        tft.drawString("Special Mode!", 5, 200, 2);
      }
    }
  }
  // tft.setRotation(3);
  // clearScreen();
}

void printLoadingAnimation()
{
  clearScreen();
  tft.drawString("Loading.", 100, 100, 4);
  delay(400);
  tft.drawString("Loading..", 100, 100, 4);
  delay(400);
  tft.drawString("Loading...", 100, 100, 4);
  delay(500);
  clearScreen();
}

void printDoorOpenScreen()
{
  printInstantAverage(lcdData.inst_kml, lcdData.inst_lph);
  printGear(lcdData.gear, lcdData.converter_lock);
  printScreenGrid(currentScreen);
  if ((wasFL_DoorOpen != isFL_DoorOpen) ||
      (wasFR_DoorOpen != isFR_DoorOpen) ||
      (wasRL_DoorOpen != isRL_DoorOpen) ||
      (wasRR_DoorOpen != isRR_DoorOpen) ||
      (wasBack_DoorOpen != isBack_DoorOpen))
  {
    // Reprint the car picture
    showCarPic();
    isDoorLockCmd = false;
    // Update previous door states
    wasFL_DoorOpen = isFL_DoorOpen;
    wasFR_DoorOpen = isFR_DoorOpen;
    wasRL_DoorOpen = isRL_DoorOpen;
    wasRR_DoorOpen = isRR_DoorOpen;
    wasBack_DoorOpen = isBack_DoorOpen;
    // Reprint doors
    isFL_DoorDisplayed = false;
    isFR_DoorDisplayed = false;
    isRL_DoorDisplayed = false;
    isRR_DoorDisplayed = false;
    isBack_DoorDisplayed = false;
  }

  // Check for state changes and update display accordingly
  if (isFL_DoorOpen && !isFL_DoorDisplayed)
  {
    showDoorFLPic();
    isFL_DoorDisplayed = true;
  }

  if (isFR_DoorOpen && !isFR_DoorDisplayed)
  {
    showDoorFRPic();
    isFR_DoorDisplayed = true;
  }

  if (isRL_DoorOpen && !isRL_DoorDisplayed)
  {
    showDoorRLPic();
    isRL_DoorDisplayed = true;
  }

  if (isRR_DoorOpen && !isRR_DoorDisplayed)
  {
    showDoorRRPic();
    isRR_DoorDisplayed = true;
  }

  if (isBack_DoorOpen && !isBack_DoorDisplayed)
  {
    showDoorBackPic();
    isBack_DoorDisplayed = true;
  }
}

void handleDoorLockUnlock()
{
  if (!isDoorLock && lcdData.speed > 4)
  {
    D_println("handleDoorLockUnlock");
    lockDoor();
  }

  /*if (lcdData.gear == 'P' && loopStartMillis - doorUnlockMills > 5000 && !isDoorLockCmd)
{
  doorUnlockMills = loopStartMillis;
  isDoorLockCmd = true;
  unLockDoor();
}*/

  if (lockButtonPressed && isMcpOk && millis() - lockUnlockPressedMillis > 500)
  {
    lockButtonPressed = false;
    mcp.digitalWrite(BUTTON_LOCK_OUTPUT_PIN, false);
  }

  if (unLockButtonPressed && isMcpOk && millis() - lockUnlockPressedMillis > 500)
  {
    unLockButtonPressed = false;
    mcp.digitalWrite(BUTTON_UNLOCK_OUTPUT_PIN, false);
  }
}

void displayScreen(Screen screen)
{

  if (!isAccOn)
  {
    printKhudaHafiz();
    return;
  }
  if (clearScreenReq)
  {
    // D_println("clearScreen done");
    clearScreenReq = false;
    clearScreen();
  }
  tft.setTextDatum(TL_DATUM);
  // D_print("current screen: ");
  // D_println(screenNames[currentScreen]);
  switch (screen)
  {
  case NATIVE_BUTTON_SCREEN:
    if (millis() - temporaryScreenMillis >= 5000 && !nativeButtonLongClick)
    {
      selectScreen(previousScreen);
      resetVariable();
      initPrvLcdData();
    }
    break;
  case DOOR_OPEN_SCREEN:
    printDoorOpenScreen();
    break;
  case TPMS_SCREEN:
    print_TPMS_screen();
    break;
  case CURRENT_TIME_SCREEN:
    print_time_screen();
    if (temporaryScreen && millis() - temporaryScreenMillis >= 5000)
    {
      temporaryScreen = false;
      selectScreen(previousScreen);
      resetVariable();
      initPrvLcdData();
    }
    break;
  case SPEED_SCREEN:
    print_Speed_screen();
    break;
  case INJECTOR_TIMING_SCREEN:
    print_Injector_Timing_screen();
    break;
  case TRIP_SCREEN:
    print_trip_screen();
    break;
  default:
    print_time_screen();
    break;
  }
  // Refresh the display
  // display.display();
}
void checkTemperature(int delay)
{
  static unsigned long lastTempMillis = 0;

  if (loopStartMillis - lastTempMillis > delay)
  {
    lastTempMillis = loopStartMillis;
    D_println("checkTemperature");
    sht.read();
    carTemperature = sht.getTemperature();
    carHumidity = sht.getHumidity();
    D_print("Humidity :");
    D_print(carHumidity);
    D_print("  Temp :");
    D_print(carTemperature);
    D_println("C");
  }
}

void checkCurrentTime(int delay)
{
  static unsigned long lastTimeMillis = 0;
  if (isRTC_Connected && loopStartMillis - lastTimeMillis > delay)
  {
    lastTimeMillis = loopStartMillis;
    // D_println("checkCurrentTime");
    now = rtc.now();
  }
}

float getVoltageFromSolarValue(float solarValue)
{
  return (128.67 - solarValue) / 55.56;
}

void setAcSolarValue(int delay)
{
  static unsigned long lastAcSolarMillis = 0;
  if (isRTC_Connected && loopStartMillis - lastAcSolarMillis > delay)
  {
    float SolarVolage = 1.0;
    lastAcSolarMillis = loopStartMillis;
    D_println("setAcSolarValue");
    if (isRTC_Connected && !rtc.lostPower() && now.hour() < 9 || now.hour() >= 19)
    {
      SolarVolage = 2.1;
    }
    else
    {
      SolarVolage = 1.0;
    }
    outputVoltageDAC(SolarVolage);
  }
}

void checkDimLights()
{
  if (isHeadLightOn != isScreenDim)
  {
    D_println("checkDimLights");
    setScreenDim(isHeadLightOn);
  }
}

void saveTPMS()
{
  static bool tpmsNeedToSaved = true;
  if (lcdData.speed == 0 && tpmsNeedToSaved)
  {
    tpmsNeedToSaved = false;
    D_println("saveTPMS");
    preferences.begin("zApp", false);
    preferences.putFloat("fl_psi", tyre_FL_PSI);
    preferences.putFloat("fr_psi", tyre_FR_PSI);
    preferences.putFloat("rl_psi", tyre_RL_PSI);
    preferences.putFloat("rr_psi", tyre_RR_PSI);
    preferences.end();
  }
  if (lcdData.speed > 0 && !tpmsNeedToSaved)
  {
    tpmsNeedToSaved = true;
  }
}
void handleLatch()
{
  // After 30 seconds of ACC being OFF, pull latch pin LOW
  if (!isAccOn && millis() - accOffTime >= 3000)
  {
    mcp.digitalWrite(ESP_LATCH_RELAY_OUTPUT_PIN, false);
  }

  /*if (!isAccOn && isDoorLock && (millis() - accOffTime) >= 100)
  {
    unLockDoor();
  }*/
}

void handleLockByAndriod()
{
  if (isLockByAndriod && millis() - lastLockByAndriodTime > 900000) // 15 minutes
  {
    preferences.begin("zApp", false);
    preferences.putBool("isLockByAndriod", true);
    preferences.end();
    delay(500);
    ESP.restart();
  }
}

static bool isOBD = false;
void setupOBD()
{
  D_println("setupOBD");
  if (isSpecialMode)
    return;

  CAN0.setCANPins((gpio_num_t)CAN_RX_PIN, (gpio_num_t)CAN_TX_PIN);
  // OBD2.setTimeout(2000);
  isOBD = OBD2.begin();
  if (!isOBD)
  {
    D_println("OBD begin failed!");
    return;
  }
}

void checkOBD2Connection()
{
  static unsigned long lastCheckOBD2Millis = 0;
  if (!isOBD && loopStartMillis - lastCheckOBD2Millis > 60000)
  {
    lastCheckOBD2Millis = loopStartMillis;
    D_println("checkOBD2Connection");
    isOBD = OBD2.begin();
  }
}

void getOBD2Data(int delay)
{
  if (!isAccOn)
  {
    return;
  }

  static unsigned long lastOBD2Millis = 0;
  if (isOBD && loopStartMillis - lastOBD2Millis > delay)
  {
    lastOBD2Millis = loopStartMillis;
    // D_println("getOBD2Data");
    OBD2Data.EngineTemp1 = OBD2.pidRead(ENGINE_COOLANT_TEMPERATURE);
  }
}

void getMeterData(int delay)
{
  static unsigned long lastMeterPacketMillis = 0;
  static unsigned long lastMeterDataMillis = 0;
  if (millis() - lastMeterDataMillis > delay)
  {
    lastMeterDataMillis = millis();
    isMeterDataReceived = false;
    // D_println("getMeterData");
    if ((millis() - lastMeterPacketMillis) > 60000)
    {
      isAirBag = 0;
      isCruiseMain = 0;
      isCruiseStart = 0;
      isImmobilizer = 0;
      isCheckEngine = 0;
      isBattery = 0;
      isOilPressure = 0;
      isSeatBelt = 0;
      FuelLevel = 0;
      lastValidFuelLevel = 0;
    }
    // Check if data is available from the LGT8F328P
    // D_println("Before: ");
    // D_println(Serial2.available());
    // int excessBytes = Serial2.available() - 21;
    // while (excessBytes > 0)
    // {
    //  Serial2.read();
    //   excessBytes--;
    // }
    // D_println("  After: ");
    // D_println(Serial2.available());

    while (Serial2.available())
    {
      String data = Serial2.readStringUntil('\n'); // Read data until a newline character
      data.trim();                                 // Remove any whitespace or newline characters
      // Ensure the data starts with "[" and ends with "]"
      if (data.startsWith("[") && data.endsWith("]") && data.length() > 5)
      {
        isMeterDataReceived = true;
        lastMeterPacketMillis = millis();
        data = data.substring(1, data.length() - 1); // Remove "[" and "]"

        // Split the data by commas and assign values to variables
        int index = 0;
        // D_println(data);
        while (data.length() > 0)
        {
          int commaIndex = data.indexOf(',');
          String value;

          if (commaIndex == -1)
          {
            value = data; // Last value
            data = "";    // Clear data
          }
          else
          {
            value = data.substring(0, commaIndex);
            data = data.substring(commaIndex + 1);
          }

          // Assign values to corresponding variables based on the index
          switch (index)
          {
          case 0:
            isAirBag = value.toInt();
            break;
          case 1:
            isCruiseMain = value.toInt();
            break;
          case 2:
            isCruiseStart = value.toInt();
            break;
          case 3:
            isImmobilizer = value.toInt();
            break;
          case 4:
            isCheckEngine = value.toInt();
            break;
          case 5:
            isBattery = value.toInt();
            break;
          case 6:
            isOilPressure = value.toInt();
            break;
          case 7:
            isSeatBelt = value.toInt();
            break;
          case 8:
            FuelLevel = value.toInt();
            if (FuelLevel > 0 && FuelLevel < 21)
            {
              lastValidFuelLevel = FuelLevel;
            }
            break;
          }
          index++;
        }
      }
    }
  }
}

bool isWebSocketConnected = false;
unsigned long lastWebSocketConnection = 0;
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                      void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    isWebSocketConnected = true;
    if (millis() - lastWebSocketConnection >= 500)
    {
      tft.drawString("***Connected****", 50, 150);
      lastWebSocketConnection = millis();
    }
#if DEBUG
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
#endif
    break;
  case WS_EVT_DISCONNECT:
    isWebSocketConnected = false;
    if (millis() - lastWebSocketConnection >= 500)
    {
      tft.drawString("**Disconnected**", 50, 150);
      lastWebSocketConnection = millis();
    }
#if DEBUG
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
#endif
    break;
  case WS_EVT_DATA:
    // handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
    break;
  case WS_EVT_ERROR:
    if (millis() - lastWebSocketConnection >= 500)
    {
      tft.drawString("*****Error******", 50, 150);
      lastWebSocketConnection = millis();
    }
    break;
  }
}

void startOBD2Sniffing()
{
  // Setup Websocket Server
  /// webSocket.begin();
  // webSocket.onEvent(onWebSocketEvent);

  webSocket.onEvent(onWebSocketEvent);
  server.addHandler(&webSocket);

  CAN0.setCANPins((gpio_num_t)CAN_RX_PIN, (gpio_num_t)CAN_TX_PIN);
  CAN0.begin(500000); // 500Kbps
  CAN0.watchFor(0x18D00000, 0xFFF00000);
}

int currentSelection = 0; // 0: Start Server, 1: Start OBD2
bool actionTriggered = false;

void handelSpecialMode()
{
  static unsigned long lastSpecialModeMillis = 0;
  static int lastSelection = -1;
  static bool lastDrawn = false;

  // Only check buttons every 200ms
  if (millis() - lastSpecialModeMillis > 200)
  {
    lastSpecialModeMillis = millis();

    if (digitalRead(BUTTON_MINUS_PIN) == LOW && !actionTriggered)
    {
      currentSelection = (currentSelection + 1) % 2; // Toggle menu option
    }

    if (digitalRead(BUTTON_PLUS_PIN) == LOW && !actionTriggered)
    {
      actionTriggered = true;

      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextSize(2);

      if (currentSelection == 0)
      {
        isOBDServer = false;
        tft.drawString("Server started.", 50, 50);
      }
      else if (currentSelection == 1)
      {
        isOBDServer = true;
        startOBD2Sniffing();
        tft.drawString("OBD2 started.", 50, 50);
      }

      tft.drawString("192.168.4.1", 50, 100);
    }

    // Update menu only if selection has changed
    if (lastSelection != currentSelection && !actionTriggered)
    {
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextSize(2);

      if (currentSelection == 0)
      {
        tft.drawString("> Start Server", 50, 50);
        tft.drawString("  Start OBD2", 50, 100);
      }
      else if (currentSelection == 1)
      {
        tft.drawString("  Start Server", 50, 50);
        tft.drawString("> Start OBD2", 50, 100);
      }

      lastSelection = currentSelection;
    }
  }
}

void broadcastObdStatus()
{
  static unsigned long lastBroadcastMillis = 0;
  // Broadcast STATUS message every second
  if ((millis() - lastBroadcastMillis) > 1000)
  {
    lastBroadcastMillis = millis();
    // Serial.println("Sending status!");
    JsonDocument jsonobj;
    String output = "";
    jsonobj["ts"] = millis();
    jsonobj["type"] = "STATUS";
    jsonobj["baudrate"] = "500000";
    jsonobj["filterId"] = "0x18D00000";
    jsonobj["filterMask"] = "0xFFF00000";
    serializeJson(jsonobj, output);
    webSocket.textAll(output);
  }
}

// Handle CAN Input
void handleCAN_RX()
{
  if (!isWebSocketConnected)
    return;
  CAN_FRAME can_message;
  if (CAN0.read(can_message))
  {
    String output = "";
    if (can_message.id == 0x18DA11F1)
    {
      output += "\n";
    }
    output += "0x";
    output += String(can_message.id, HEX);
    if (can_message.extended)
    {
      output += can_message.rtr ? " [ERTR]" : " [EXT] ";
    }
    else
    {
      output += can_message.rtr ? " [RTR] " : " [STD] ";
    }

    output += " [";
    output += String(can_message.length);
    output += "] {";

    for (int i = 0; i < can_message.length; i++)
    {
      if (i != 0)
        output += ":";
      output += String(can_message.data.byte[i], HEX);
    }
    output += "}\n";

    // Send to WebSocket clients
    webSocket.textAll(output);

    D_println("data sent");
  }
}

void printResetReason()
{
  // Log the reset reason
  esp_reset_reason_t resetReason = esp_reset_reason();
  switch (resetReason)
  {
  case ESP_RST_POWERON:
    D_println("Power on reset");
    break;
  case ESP_RST_EXT:
    D_println("External reset");
    break;
  case ESP_RST_SW:
    D_println("Software reset");
    break;
  case ESP_RST_PANIC:
    D_println("Panic reset");
    break;
  case ESP_RST_INT_WDT:
    D_println("Reset due to interrupt watchdog");
    break;
  case ESP_RST_TASK_WDT:
    D_println("Reset due to task watchdog");
    break;
  case ESP_RST_WDT:
    D_println("Reset due to RTC watchdog");
    break;
  case ESP_RST_DEEPSLEEP:
    D_println("Reset due to deep sleep wakeup");
    break;
  case ESP_RST_BROWNOUT:
    D_println("Brownout reset");
    break;
  case ESP_RST_SDIO:
    D_println("SDIO reset");
    break;
  default:
    D_println("Unknown reset reason");
    break;
  }
}

void setupWatchDog()
{
  // --- TASK WDT ---
  esp_task_wdt_init(TIMEOUT_SEC, true); // Panic on timeout = true
  esp_task_wdt_add(NULL);               // Add current task (loopTask)

  // --- RTC WDT ---
  rtc_wdt_protect_off();
  rtc_wdt_disable();

  rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_SYSTEM);
  // rtc_wdt_set_time(RTC_WDT_STAGE0, TIMEOUT_SEC * APB_CLK); // 30 seconds
  rtc_wdt_set_time(RTC_WDT_STAGE0, (uint64_t)TIMEOUT_SEC * (uint64_t)APB_CLK); // 30 seconds

  rtc_wdt_enable();
  rtc_wdt_protect_on();

  D_println("Both Task WDT and RTC WDT enabled.");
}

#if BLE
// BLE callback class
class LockUnlockCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pChar)
  {
    std::string value = pChar->getValue();
    Serial.print("Received BLE command: ");
    Serial.println(value.c_str());

    if (value == "lock")
    {
      Serial.println("Door Locked");
      lockDoor();
      pStatusCharacteristic->setValue("locked");
      pStatusCharacteristic->notify();
    }
    else if (value == "unlock")
    {
      Serial.println("Door Unlocked");
      lockDoor();
      pStatusCharacteristic->setValue("unlocked");
      pStatusCharacteristic->notify();
    }
  }
};

// Call this once in setup()
void initBLE()
{
  Serial.println("initBLE()");
  BLEDevice::init("ESP32_BLE_Lock");
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Command write characteristic
  pCharacteristic = pService->createCharacteristic(
      CMD_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new LockUnlockCallbacks());

  // Status notify characteristic
  pStatusCharacteristic = pService->createCharacteristic(
      STATUS_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_NOTIFY);

  pStatusCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("BLE Lock/Unlock service started.");
}
#endif

const int MESSAGE_LENGTH = 17; // Length of each message
uint8_t messageBuffer[MESSAGE_LENGTH];
bool isCapturing = false;
int messageIndex = 0;
uint8_t previousFirstByte = 0;         // To track the sequence of the first byte
unsigned long lastMessageTime = 0;     // To track the time of the last complete message
const unsigned long MESSAGE_DELAY = 2; // Expected delay between messages in milliseconds (use 3ms for safety)

void setup(void)
{
#if RESET_FLASH
  resetFlash();
#endif

  unsigned long setupStart = millis();

  pinMode(LCD_LIGHT_PIN, OUTPUT);
  digitalWrite(LCD_LIGHT_PIN, 0);
  D_SerialBegin(115200);
  // Serial.begin(115200);
  delay(100);
  // Serial.setDebugOutput(true);
  D_println("******************** Setup Start *******************");
  printResetReason();
  // Serial2.setRxBufferSize(32);
  Serial2.begin(9600, SERIAL_8N1, 16, -1); // Meter Uart
                                           // Serial.printf("Flash Chip Size: %d bytes\n", ESP.getFlashChipSize());
  // isSpecialMode = true;                    // ztest
  readSetting();
  setupWatchDog();
  setupMcp();
  setupPin();
  if (isLockByAndriod)
  {
    isDoorLock = true;
    lastLockByAndriodTime = millis();
    mcp.digitalWrite(BUTTON_LOCK_OUTPUT_PIN, true);
    delay(500);
    mcp.digitalWrite(BUTTON_LOCK_OUTPUT_PIN, false);
  }
  setupPWM();
  esp_task_wdt_reset();
  setupButtons();
  setupScreen();
  printPray();
  SetupESP_NOW();
  esp_task_wdt_reset();
  setupTPMS();
  setupRTC();
  setupOBD();
  esp_task_wdt_reset();
  Wire.begin(21, 22);
  sht.begin();
  resetVariable();
  initPrvLcdData();
  send_page_number(currentTrip, 1);
  esp_task_wdt_reset();
  startServer();
  setupTelnetServer();
  // startMeasurementAHT20();
  if (isSpecialMode)
  {
    clearScreen();
  }
  else
  {
    printLoadingAnimation();
  }
  outputVoltageDAC(1.2);

#if BLE
  initBLE(); // Initialize BLE lock/unlock service
#endif

  /*if (rtc.lostPower())
  {
    Serial.println("RTC lost power, setting the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }*/
  // updateTPMSScreen = true;

  unsigned long setupEnd = millis();
  D_println("***************************************************");
  D_print("              !Setup Done! time(ms): ");
  D_println(setupEnd - setupStart);
  D_println("***************************************************");

  /*isSpecialMode = true;
  actionTriggered = true;
  isOBDServer = true;
  startOBD2Sniffing();
  startServer(isOBDServer);*/

  //  delay(900000);
  //    isSpecialMode = true;
}
/*
const char *messages[] = {
    "0x18DA11F1 [8] <30:0:10:0:0:0:0:0>",
    "0x18DAF111 [8] <21:7D:E0:0:0:0:2D:14>",
    "0x18DAF111 [8] <22:34:6E:36:6F:8F:61:8F>",
    "0x18DAF111 [8] <23:61:0:0:0:80:7A:FE>",
    "0x18DAF111 [8] <24:0:0:40:0:0:FF:0>",
    "0x18DAF111 [8] <25:F:0:17:0:20:0:5F>",
    "0x18DAF111 [8] <26:56:0:0:0:0:0:0>",
    "0x18DAF111 [8] <27:0:0:0:0:0:0:0>",
    "0x18DAF111 [8] <28:0:0:55:55:55:55:55>",
    "0x18DA11F1 [8] <3:22:26:10:0:0:0:0>",
    "0x18DAF111 [8] <10:39:62:26:10:FF:FD:FF>",
    "0x18DA11F1 [8] <30:0:10:0:0:0:0:0>",
    "0x18DAF111 [8] <21:7D:E0:0:0:0:2D:14>"};

const int messageCount = sizeof(messages) / sizeof(messages[0]);
int currentIndex = 0;
unsigned long lastSendTime = 0;
*/


void calculateAverageSpeed(int delay)
{
  static unsigned long lasCalAvgMillis = 0;
  if (millis() - lasCalAvgMillis > delay)
  {
    lasCalAvgMillis = millis();
    unsigned long timeDelta = currentEngineRunTime - lastUpdateTime;
    lastUpdateTime = currentEngineRunTime;
    if (lcdData.speed > 0.0)
    {
      totalMovingTimeMs += timeDelta;
      float timeDeltaHours = timeDelta / 3600000.0;
      totalDistanceKm += (lcdData.speed * timeDeltaHours);
      if (totalMovingTimeMs > 0)
      {
        float totalMovingHours = totalMovingTimeMs / 3600000.0;
        avg_speed = totalDistanceKm / totalMovingHours;
      }
    }
    else
    {
      idleCarRunTimeMs += timeDelta;
    }
  }
}

void loop()
{
  // Feed both watchdogs
  esp_task_wdt_reset();
  loopStartMillis = millis();

  if (isSpecialMode && !isLockByAndriod)
  {
    handelSpecialMode();
    if (isOBDServer)
    {
      // webSocket.loop();
      webSocket.cleanupClients();
      // broadcastObdStatus();
      handleCAN_RX();
    }
  }
  else
  {
    dnsServer.processNextRequest();
    handleTelnet();
    handleLockByAndriod();
    // if(!isLockByAndriodReboot){
    btnPlus.loop();
    btnMinus.loop();
    processTPMS();
    processInputPins(300);
    handleDoorLockUnlock();
    checkDimLights();
    checkTemperature(3000);
    checkCurrentTime(1000);
    getMeterData(150);
    checkOBD2Connection();
    getOBD2Data(5000);
    calculateEngineRunningTime(300);
    calculateAverageSpeed(300);
    printHeader(1000);
    displayScreen(currentScreen);
    printCruiseIcon();
    saveTPMS();
    handleLatch();
    setAcSolarValue(60000);
    //}
  }
  // D_println("<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>");
  delay(0);
}
