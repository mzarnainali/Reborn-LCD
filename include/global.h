// TPMS Definitions
#define CC1101_CS_PIN 5
#define CC1101_GDO0_PIN 4 // 13
#define CC1101_SCK_PIN 18
#define CC1101_MISO_PIN 19
#define CC1101_MOSI_PIN 23
// TPMS sensor IDs
/*#define ID_FL 0x1D2C01UL
#define ID_FR 0x1F9E17UL
#define ID_RL 0x1FAA50UL
#define ID_RR 0x1D500EUL*/

#define ID_FL_DEF 0x1F9E17UL
#define ID_FR_DEF 0x1D500EUL
#define ID_RL_DEF 0x1FAA50UL
#define ID_RR_DEF 0x1D2C01UL 

// Default IDs
uint32_t ID_FL ;
uint32_t ID_FR ;
uint32_t ID_RL ;
uint32_t ID_RR ;

#define TIMEOUT 120000 // Timeout in ms
#define CHECKLENGTH 16
#define TPMS_DEBUG_ERR
// LCD Definitions
#define LCD_LIGHT_PIN 27
// DOOR Definitions
#define DOOR_FL_PIN 8
#define DOOR_FR_PIN 9
#define DOOR_RL_PIN 10
#define DOOR_RR_PIN 11
#define DOOR_BACK_PIN 12
// DIM LIGHT Definitions
#define HEAD_LIGHT_PIN 13
// TECHO METER Definitions
// LOCK Definitions
#define BUTTON_LOCK_OUTPUT_PIN 6
#define BUTTON_UNLOCK_OUTPUT_PIN 14
// BUTTON Definitions
#define BUTTON_PLUS_PIN 34
#define BUTTON_MINUS_PIN 35
// BUTTON Output Definitions
#define BUTTON_PLUS_OUTPUT_PIN 7
#define BUTTON_MINUS_OUTPUT_PIN 15
// TIME Module Pinout
#define RTC_IC2_ADDRESS 0x68
#define RTC_EEPROM_ADDRESS 0x57

#define ESP_LATCH_RELAY_OUTPUT_PIN 5
#define CAR_ACC_VOLTAGE_PIN 4

#define AC_SUN_SENSOR_PIN 25

bool isHeadLightOn = false;
bool isScreenDim = false;

bool isFL_DoorOpen = false, wasFL_DoorOpen = false, isFL_DoorDisplayed = false;
bool isFR_DoorOpen = false, wasFR_DoorOpen = false, isFR_DoorDisplayed = false;
bool isRL_DoorOpen = false, wasRL_DoorOpen = false, isRL_DoorDisplayed = false;
bool isRR_DoorOpen = false, wasRR_DoorOpen = false, isRR_DoorDisplayed = false;
bool isBack_DoorOpen = false, wasBack_DoorOpen = false, isBack_DoorDisplayed = false;

bool isDoorLock = false;
bool showDoorOpenScreen = false;

float tyre_FL_PSI = 0;
float tyre_FR_PSI = 0;
float tyre_RL_PSI = 0;
float tyre_RR_PSI = 0;

int tyre_FL_temp = 0;
int tyre_FR_temp = 0;
int tyre_RL_temp = 0;
int tyre_RR_temp = 0;

unsigned long tyre_FL_time = 0;
unsigned long tyre_FR_time = 0;
unsigned long tyre_RL_time = 0;
unsigned long tyre_RR_time = 0;

bool anyTyrePressureChange = false;

unsigned long loopStartMillis = 0;

bool showTPMSScreen = false;

bool isDoorLockCmd = false;
bool isDoorUnLockCmd = false;

enum Screen
{
  CURRENT_TIME_SCREEN,
  TPMS_SCREEN,
  SPEED_SCREEN,
  TRIP_SCREEN,
  DOOR_OPEN_SCREEN,
  NATIVE_BUTTON_SCREEN,
  INJECTOR_TIMING_SCREEN
};

/*
const char *screenNames[] = {
    "CURRENT_TIME_SCREEN",
    "TPMS_SCREEN",
    "SPEED_SCREEN",
    "TRIP_SCREEN",
    "DOOR_OPEN_SCREEN",
    "NATIVE_BUTTON_SCREEN",
    "INJECTOR_TIMING_SCREEN"};
*/


byte btnCounter = 0;
int lowPSI = 20;
bool isTyrePressureLow = false;

// OBD Data
typedef struct OBD2_DATA
{
  float EngineTemp1 = 0.0f;
  float IntakeTemp = 0.0f;
  float ThrottlePosition = 0.0f;
  float CommandedEGR = 0.0f;
  float EGRError = 0.0f;
  float Voltage = 0.0f;
  int RPM = 0;
  int ShortTrim = 0;
  int TimingAdvance = 0;
} OBD2_DATA;

// hw_timer_t *timer = NULL;

// Define the pins for indicators
const uint8_t PIN_AIRBAG = 2;
const uint8_t PIN_CRUISE_MAIN = 3;
const uint8_t PIN_CRUISE_START = 4;
const uint8_t PIN_IMMOBILIZER = 5;
const uint8_t PIN_CHECK_ENGINE = 6;
const uint8_t PIN_BATTERY = 7;
const uint8_t PIN_OIL_PRESSURE = 8;
const uint8_t PIN_SEAT_BELT = 9;

bool isAirBag = false;
bool isCruiseMain = false;
bool isCruiseStart = false;
bool isImmobilizer = false;
bool isCheckEngine = false;
bool isBattery = false;
bool isOilPressure = false;
bool isSeatBelt = false;

bool prevCruiseStart = false;
bool prevCruiseMain = false;

int FuelLevel = 0;
int lastValidFuelLevel = 0;
bool isMeterDataReceived = false;

bool lockButtonPressed = false;
bool unLockButtonPressed = false;
unsigned lockUnlockPressedMillis = 0;

bool isSpecialMode = false;

unsigned long accOffTime = 0; // Timestamp when ACC turned off
bool isAccOn = false;
bool prvIsAccOn = false;
/*
const uint8_t PIN_AIRBAG = 2;
const uint8_t PIN_CRUISE_MAIN = 3;
const uint8_t PIN_CRUISE_START = 4;
const uint8_t PIN_IMMOBILIZER = 5;
const uint8_t PIN_CHECK_ENGINE = 6;
const uint8_t PIN_BATTERY = 7;
const uint8_t PIN_OIL_PRESSURE = 8;
const uint8_t PIN_SEAT_BELT = 9;
*/

/*
#define CC1101_SCK     14
#define CC1101_MISO    26
#define CC1101_MOSI    13
#define CC1101_CS      17
#define CC1101_GDO0    16
#define CC1101_GDO2    34

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15
#define TFT_DC   33
#define TFT_RST  27
#define TOUCH_CS 12

*/