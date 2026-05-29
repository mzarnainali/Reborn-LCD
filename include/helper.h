#include <Arduino.h>

byte decode_manchester_nibble(byte b, bool &error)
{
  switch (b)
  {
  case 0x55:
    return 0x0;
  case 0x56:
    return 0x1;
  case 0x59:
    return 0x2;
  case 0x5A:
    return 0x3;
  case 0x65:
    return 0x4;
  case 0x66:
    return 0x5;
  case 0x69:
    return 0x6;
  case 0x6A:
    return 0x7;
  case 0x95:
    return 0x8;
  case 0x96:
    return 0x9;
  case 0x99:
    return 0xA;
  case 0x9A:
    return 0xB;
  case 0xA5:
    return 0xC;
  case 0xA6:
    return 0xD;
  case 0xA9:
    return 0xE;
  case 0xAA:
    return 0xF;
  default:
    error = true;
    return 0x00;
  }
}

byte decode_manchester_byte(byte bnh, byte bnl, bool &error)
{
  byte ret = 0;
  ret = (decode_manchester_nibble(bnh, error) << 4);
  ret = ret | decode_manchester_nibble(bnl, error);
  return ret;
}

String getIntegerPart(float number)
{
  int integerPart = static_cast<int>(number); // Extract the integer part
  return String(integerPart);                 // Convert to String and return
}

String getFractionalPart(float number, unsigned int decimalPlaces = 1)
{
  float fractionalPart = number - static_cast<int>(number);                       // Extract fractional part
  float roundingFactor = pow(10, decimalPlaces);                                  // Calculate rounding factor
  int roundedFraction = static_cast<int>(round(fractionalPart * roundingFactor)); // Round the fractional part
  String result = String(roundedFraction);
  // Ensure leading zeros are added if necessary
  while (result.length() < decimalPlaces)
  {
    result = "0" + result;
  }
  return "." + result;
}

void printTime(DateTime now)
{
  // Display the current date and time
  D_print("Date & Time: ");
  D_print(now.year(), DEC);
  D_print('/');
  D_print(now.month(), DEC);
  D_print('/');
  D_print(now.day(), DEC);
  D_print(" ");
  D_print(now.hour(), DEC);
  D_print(':');
  D_print(now.minute(), DEC);
  D_print(':');
  D_print(now.second(), DEC);
  D_println();
}

// Struct to store display function and condition
typedef void (*PicFunction)();
struct PicDisplay
{
  PicFunction func;
  bool *condition;
};



// The basic figure drawing function are as follows.
/*
    fillScreen    (                color);
    drawPixel     ( x, y         , color);
    drawFastVLine ( x, y   , h   , color);
    drawFastHLine ( x, y, w      , color);
    drawRect      ( x, y, w, h   , color);
    fillRect      ( x, y, w, h   , color);
    drawRoundRect ( x, y, w, h, r, color);
    fillRoundRect ( x, y, w, h, r, color);
    drawCircle    ( x, y      , r, color);
    fillCircle    ( x, y      , r, color);
    drawEllipse   ( x, y, rx, ry , color);
    fillEllipse   ( x, y, rx, ry , color);
    drawLine      ( x0, y0, x1, y1        , color);
    drawTriangle  ( x0, y0, x1, y1, x2, y2, color);
    fillTriangle  ( x0, y0, x1, y1, x2, y2, color);
    drawBezier    ( x0, y0, x1, y1, x2, y2, color);         // 3-point Bezier
    drawBezier    ( x0, y0, x1, y1, x2, y2, x3, y3, color); // 4-point Bezier
    drawArc       ( x, y, r0, r1, angle0, angle1, color);
    fillArc       ( x, y, r0, r1, angle0, angle1, color);
  */

//********** CHANGE PIN FUNCTION  TO GPIO **********
// GPIO 1 (TX) swap the pin to a GPIO.
// pinMode(1, FUNCTION_3);
// GPIO 3 (RX) swap the pin to a GPIO.
// pinMode(3, FUNCTION_3);
//**************************************************

//********** CHANGE PIN FUNCTION  TO TX/RX **********
// GPIO 1 (TX) swap the pin to a TX.
// pinMode(1, FUNCTION_0);
// GPIO 3 (RX) swap the pin to a RX.
// pinMode(3, FUNCTION_0);
//***************************************************