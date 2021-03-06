// // ESP ambient light control firmware for Avante AD
// // created : Jan 22th, 2022
// // datastructure
// data saved in EEPROM with following sequence:
// [Master Brightness, AccSync Enable, AccSync Sensitivity, Color1 Pos, color1R, color1G, Color1B...(Let's take Master Color as color5)]

//  0 mater brightness
//  1 AccSync
//  2 Sensitivity
//  3 Color 1 pos
//  4 Color 1 r
//  5 Color 1 g
//  6 Color 1 b
//  7 Color 2 pos
//  8 Color 2 r
//  9 Color 2 g
//  10  Color 2 b
//  11  Color 3 pos
//  12  Color 3 r
//  13  Color 3 g
//  14  Color 3 b
//  15  Color 4 pos
//  16  Color 4 r
//  17  Color 4 g
//  18  Color 4 b
//  19  Color 5(MasterColor)  pos
//  20  Color 5(MasterColor)  r
//  21  Color 5(MasterColor)  g
//  22  Color 5(MasterColor)  b
//  23  Color 6(Welcome light)  pos
//  24  Color 6(Welcome light)  r
//  25  Color 6(Welcome light)  g
//  26  Color 6(Welcome light)  b

#include "FastLED.h" // FastLED library. Preferably the latest copy of FastLED 2.1.
#include "colorutils.h"
#include <Arduino.h>
#include <Preferences.h>
#include <EEPROM.h>

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define SERIAL_NO 01
// Fixed global definitions.

#define PERIPHERAL_NAME "ESP Ambient"
#define SERVICE_UUID "B96ED503-7609-4D70-B713-AE8DEBE6AB07"
#define CHARACTERISTIC_OUTPUT_UUID "643954A4-A6CC-455C-825C-499190CE7DB0"

#define COLOR_UUID "D4705742-24F6-4AE6-B807-BF222FD743E7"      // [pos, num, r,g,b]
#define ACC_SYNC_UUID "D4705742-24F6-4AE6-B807-BF222FD743E7"   // [enable, sensitivity]
#define BRIGHTNESS_UUID "2EF75BBC-7FDB-4371-BAE0-9A6D0C563A89" // [brightness]
// [Master Brightness, AccSync Enable, AccSync Sensitivity, Color1 Pos, color1R, color1G, Color1B...(Let's take Master Color as color5 and Welcome Light as Color6)]
#define DATA_MGMT_UUID "C34C5C14-64DF-4B56-8FBC-168B17A3FF5E" //
#define RESTART_UUID "E494E4A6-2DBE-4FEB-81B1-25A2DAF1AFBD"

#define MASTER_UUID "C1AB2C55-7914-4140-B85B-879C5E252FE5"
#define TESTCOLOR_UUID "E494E4A6-2DBE-4FEB-81B1-25A2DAF1AFBD"

#define DATA_PIN 18     // Data pin to connect to the strip.
#define COLOR_ORDER BGR // Are they RGB, GRB or what??
#define LED_TYPE WS2813 // Don't forget to change LEDS.addLeds
#define NUM_LEDS 59     // Number of LED's.

#define NEOPIXEL_DOOR_NUM 29
#define NEOPIXEL_LEFT_NUM 12
#define NEOPIXEL_RIGHT_NUM 66
#define NEOPIXEL_TOTAL 136

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

//#include "lib_NeoPixel/Adafruit_NeoPixel.h"
//#include "color_space.h"
//#include "main.h"

// rgb rVal[4];
// hsv hVal[4];

// Initialize changeable global variables.
uint8_t max_bright = 255; // Overall brightness definition. It can be changed on the fly.

struct CRGB leds[NUM_LEDS]; // Initialize our LED array.

Preferences prfs;

// Current value of output characteristic persisted here
static uint8_t outputData[1] = {200};

// BT Section //////////////////////////////////////////////////////////////////////////////////

// Output characteristic is used to send the response back to the connected phone
BLECharacteristic *pOutputChar;

uint8_t mBrightness = 0;
uint8_t mMode = 0;

// data
uint8_t sBrightness = 15;
uint8_t sMultiColor = 3;
uint8_t sAccSync[2] = {
    1,
};
uint8_t sColorPos[6] = {
    1,
};
CRGB sColor[6] = {
    CRGB(1, 0, 1),
};

void gradRGB(CRGB *leds,
             uint16_t pos1, CRGB pos1color,
             uint16_t pos2, CRGB pos2color, uint16_t pos3, CRGB pos3color, uint16_t pos4, CRGB pos4color);

void isnotZero(Preferences prfs, const char *key, uint32_t value)
{
  if (value != 0)
  {
    prfs.putUInt(key, value);
  }
}

// Class defines methods called when a device connects and disconnects from the service
class ServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    Serial.println("BLE Client Connected");

  }
  void onDisconnect(BLEServer *pServer)
  {
    BLEDevice::startAdvertising();
    Serial.println("BLE Client Disconnected");
  }
};

// class ColorCallbacks : public BLECharacteristicCallbacks
// {
//   void onWrite(BLECharacteristic *pCharWriteState)
//   {
//     uint8_t *inputValues = pCharWriteState->getData(); // lightPos, lightNum, r,g,b

//     FastLED.clear();

//     if (inputValues[0] == 255)
//     { // Master Color
//       sColor[4].r = inputValues[2];
//       sColor[4].g = inputValues[3];
//       sColor[4].b = inputValues[4];
//       fill_solid(leds, NUM_LEDS, sColor[4]);
//       sMultiColor = 0;
//     }

//     else if (inputValues[0] == 254)
//     { // Welcome Light
//       sColor[5].r = inputValues[2];
//       sColor[5].g = inputValues[3];
//       sColor[5].b = inputValues[4];
//     }

//     else
//     {
//       sMultiColor = 1;

//       switch (inputValues[1])
//       {
//       case 1:
//         sColorPos[0] = inputValues[0];
//         sColor[0].r = inputValues[2];
//         sColor[0].g = inputValues[3];
//         sColor[0].b = inputValues[4];
//         break;

//       case 2:
//         sColorPos[1] = inputValues[0];
//         sColor[1].r = inputValues[2];
//         sColor[1].g = inputValues[3];
//         sColor[1].b = inputValues[4];
//         break;

//       case 3:
//         sColorPos[2] = inputValues[0];
//         sColor[2].r = inputValues[2];
//         sColor[2].g = inputValues[3];
//         sColor[2].b = inputValues[4];
//         break;

//       case 4:
//         sColorPos[3] = inputValues[0];
//         sColor[3].r = inputValues[2];
//         sColor[3].g = inputValues[3];
//         sColor[3].b = inputValues[4];
//         break;

//       default:
//         break;
//       }

//       gradRGB(leds, sColorPos[0], sColor[0], sColorPos[1], sColor[1], sColorPos[2], sColor[2], sColorPos[3], sColorPos[3]);
//       Serial.println("colorCallbacks");
//     }

//     FastLED.show();
//     pOutputChar->setValue((uint8_t *)outputData, 1);
//     pOutputChar->notify();
//   }
// };

class AccSyncCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharWriteState)
  {
    uint8_t *inputValues = pCharWriteState->getData();

    sAccSync[0] = inputValues[0];
    sAccSync[1] = inputValues[1];

    Serial.print(" AccSync : ");

    Serial.print(String(inputValues[0]) + " " + String(inputValues[1]));
    Serial.println("");

    pOutputChar->setValue((uint8_t *)outputData, 1);
    pOutputChar->notify();
  }
};

class BrightnessCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharWriteState)
  {
    uint8_t *inputValues = pCharWriteState->getData();
    sBrightness = inputValues[0];

    Serial.print(" Brightness : ");

    Serial.print(String(inputValues[0]));
    Serial.println("");

    FastLED.setBrightness(inputValues[0]);
    FastLED.show();
    pOutputChar->setValue((uint8_t *)outputData, 1);
    pOutputChar->notify();
  }
};

class RestartCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharWriteState)
  {
    ESP.restart();
    pOutputChar->setValue((uint8_t *)outputData, 1);
    pOutputChar->notify();
  }
};

class DataMgmtCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharWriteState)
  {
    uint8_t *req = pCharWriteState->getData();

    Preferences preference; // We'll gonna save the data into EEPROM.
    preference.begin("my-app", false);
    // //save
    if (sBrightness != 0)
      preference.putUInt("Brightness", sBrightness);
    if (sAccSync[0] != 0)
      preference.putUInt("AccSync", sAccSync[0]);
    if (sAccSync[1] != 0)
      preference.putUInt("Sensitivity", sAccSync[1]);
    if (sMultiColor != 0)
      preference.putUInt("MultiColor", sMultiColor);
    if (sColorPos[0] != 0)
      preference.putUInt("Color1Pos", sColorPos[0]);
    if (sColor[0].r != 0)
      preference.putUInt("Color1R", sColor[0].r);
    if (sColor[0].g != 0)
      preference.putUInt("Color1G", sColor[0].g);
    if (sColor[0].b != 0)
      preference.putUInt("Color1B", sColor[0].b);
    if (sColorPos[1] != 0)
      preference.putUInt("Color2Pos", sColorPos[1]);
    if (sColor[1].r != 0)
      preference.putUInt("Color2R", sColor[1].r);
    if (sColor[1].g != 0)
      preference.putUInt("Color2G", sColor[1].g);
    if (sColor[1].b != 0)
      preference.putUInt("Color2B", sColor[1].b);
    if (sColorPos[2] != 0)
      preference.putUInt("Color3Pos", sColorPos[2]);
    if (sColor[2].r != 0)
      preference.putUInt("Color3R", sColor[2].r);
    if (sColor[2].g != 0)
      preference.putUInt("Color3G", sColor[2].g);
    if (sColor[2].b != 0)
      preference.putUInt("Color3B", sColor[2].b);
    if (sColorPos[3] != 0)
      preference.putUInt("Color4Pos", sColorPos[3]);
    if (sColor[3].r != 0)
      preference.putUInt("Color4R", sColor[3].r);
    if (sColor[3].g != 0)
      preference.putUInt("Color4G", sColor[3].g);
    if (sColor[3].b != 0)
      preference.putUInt("Color4B", sColor[3].b);
    if (sColorPos[4] != 0)
      preference.putUInt("Color5Pos", sColorPos[4]);
    if (sColor[4].r != 0)
      preference.putUInt("Color5R", sColor[4].r);
    if (sColor[4].g != 0)
      preference.putUInt("Color5G", sColor[4].g);
    if (sColor[4].b != 0)
      preference.putUInt("Color5B", sColor[4].b);
    if (sColorPos[4] != 0)
      preference.putUInt("Color5Pos", sColorPos[4]);
    if (sColor[5].r != 0)
      preference.putUInt("Color6R", sColor[5].r);
    if (sColor[5].g != 0)
      preference.putUInt("Color6G", sColor[5].g);
    if (sColor[5].b != 0)
      preference.putUInt("Color6B", sColor[5].b);
    prfs.end();

    Serial.println("saved:  |  brightness : " + String(preference.getUInt("Brightness")) + "accsync : " + String(preference.getUInt("AccSync")) + "sense : " + String(preference.getUInt("Sensitivity")) + " multicolor : " + String(preference.getUInt("MultiColor")));

    Serial.println("data Saved Successfully.");

    pOutputChar->setValue((uint8_t *)outputData, 1);
    pOutputChar->notify();
  }
};

class MasterCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharWriteState)
  {
    uint8_t *inputValues = pCharWriteState->getData(); // lightPos, lightNum, r,g,b

    Serial.print(inputValues[0]);
    Serial.println(inputValues[1]);


    // Save Datas Here
    if (inputValues[0] == 1)
    {
      prfs.putUInt("brightness", mBrightness);
      prfs.putUInt("mode", mMode);
      prfs.putUInt("0R", sColor[0].r); // welcome
      prfs.putUInt("0G", sColor[0].g);
      prfs.putUInt("0B", sColor[0].b);
      prfs.putUInt("1R", sColor[1].r); // single
      prfs.putUInt("1G", sColor[1].g);
      prfs.putUInt("1B", sColor[1].b);
      prfs.putUInt("2R", sColor[2].r); // grad1
      prfs.putUInt("2G", sColor[2].g);
      prfs.putUInt("2B", sColor[2].b);
      prfs.putUInt("3R", sColor[3].r); // grad2
      prfs.putUInt("3G", sColor[3].g);
      prfs.putUInt("3B", sColor[3].b);

      prfs.end();

      fill_solid(leds,NUM_LEDS, CRGB(255,255,0));
      FastLED.setBrightness(128);
      FastLED.show();
      delay(100);
      FastLED.setBrightness(0);
      FastLED.show();
      delay(100);
      FastLED.setBrightness(128);
      FastLED.show();
      delay(100);
      FastLED.setBrightness(0);
      FastLED.show();
      delay(150);
      ESP.restart();
    }
    
    mBrightness = inputValues[1];
    FastLED.setBrightness(mBrightness);
    FastLED.show();

    pOutputChar->setValue((uint8_t *)outputData, 1);
    pOutputChar->notify();
  }
};

class ColorCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharWriteState)
  {
    uint8_t *inputValues = pCharWriteState->getData(); // lightPos, lightNum, r,g,b

    // code | 0 : welcome, 1:single, 2: gradient1, 3:gradient2

    Serial.print(inputValues[0]);
    Serial.print("\t");
    Serial.println(inputValues[1]);
    Serial.print("\t");
    Serial.println(inputValues[2]);
    Serial.print("\t");
    Serial.println(inputValues[3]);

    mMode = inputValues[0];
    if (inputValues[0] == 0)
    {
      sColor[0].r = inputValues[1];
      sColor[0].g = inputValues[2];
      sColor[0].b = inputValues[3];
      fill_solid(leds, NUM_LEDS, CRGB(inputValues[1], inputValues[2], inputValues[3]));
    }
    else if (inputValues[0] == 1)
    {
      sColor[1].r = inputValues[1];
      sColor[1].g = inputValues[2];
      sColor[1].b = inputValues[3];

      fill_solid(leds, NUM_LEDS, CRGB(inputValues[1], inputValues[2], inputValues[3]));
    }
    else if (inputValues[0] == 2)
    {
      sColor[2].r = inputValues[1];
      sColor[2].g = inputValues[2];
      sColor[2].b = inputValues[3];

      fill_gradient_RGB(leds, NUM_LEDS, sColor[2], sColor[3]);
    }
    else if (inputValues[0] == 3)
    {
      sColor[3].r = inputValues[1];
      sColor[3].g = inputValues[2];
      sColor[3].b = inputValues[3];

      fill_gradient_RGB(leds, NUM_LEDS, sColor[2], sColor[3]);
    }

    FastLED.setBrightness(mBrightness);
    FastLED.show();
    delay(5);

    pOutputChar->setValue((uint8_t *)outputData, 1);
    pOutputChar->notify();
  }
};

// A Function that enables Multiple Color gradient
void gradRGB(CRGB *leds,
             uint16_t pos1, CRGB pos1color,
             uint16_t pos2, CRGB pos2color, uint16_t pos3, CRGB pos3color, uint16_t pos4, CRGB pos4color)
{
  // if the points are in the wrong order, straighten them
  if (pos2 < pos1)
  {
    uint16_t t = pos2;
    CRGB tc = pos2color;
    pos2color = pos1color;
    pos2 = pos1;
    pos1 = t;
    pos1color = tc;
  }
  if (pos3 < pos2)
  {
    uint16_t t = pos3;
    CRGB tc = pos3color;
    pos3color = pos2color;
    pos3 = pos2;
    pos2 = t;
    pos2color = tc;
  }

  if (pos4 < pos3)
  {
    uint16_t t = pos4;
    CRGB tc = pos4color;
    pos4color = pos3color;
    pos4 = pos3;
    pos3 = t;
    pos3color = tc;
  }

  saccum87 rdistance87;
  saccum87 gdistance87;
  saccum87 bdistance87;
  saccum87 rdistance98;
  saccum87 gdistance98;
  saccum87 bdistance98;
  saccum87 rdistance09;
  saccum87 gdistance09;
  saccum87 bdistance09;

  rdistance87 = (pos2color.r - pos1color.r) << 7;
  gdistance87 = (pos2color.g - pos1color.g) << 7;
  bdistance87 = (pos2color.b - pos1color.b) << 7;

  rdistance98 = (pos3color.r - pos2color.r) << 7;
  gdistance98 = (pos3color.g - pos2color.g) << 7;
  bdistance98 = (pos3color.b - pos2color.b) << 7;

  rdistance09 = (pos4color.r - pos3color.r) << 7;
  gdistance09 = (pos4color.g - pos3color.g) << 7;
  bdistance09 = (pos4color.b - pos3color.b) << 7;

  uint16_t pixeldistance1 = pos2 - pos1;
  int16_t divisor1 = pixeldistance1 ? pixeldistance1 : 1;

  uint16_t pixeldistance2 = pos3 - pos2;
  int16_t divisor2 = pixeldistance2 ? pixeldistance2 : 1;

  uint16_t pixeldistance3 = pos4 - pos3;
  int16_t divisor3 = pixeldistance3 ? pixeldistance3 : 1;

  saccum87 rdelta87 = rdistance87 / divisor1;
  saccum87 gdelta87 = gdistance87 / divisor1;
  saccum87 bdelta87 = bdistance87 / divisor1;

  saccum87 rdelta98 = rdistance98 / divisor2;
  saccum87 gdelta98 = gdistance98 / divisor2;
  saccum87 bdelta98 = bdistance98 / divisor2;

  saccum87 rdelta09 = rdistance09 / divisor3;
  saccum87 gdelta09 = gdistance09 / divisor3;
  saccum87 bdelta09 = bdistance09 / divisor3;

  rdelta87 *= 2;
  gdelta87 *= 2;
  bdelta87 *= 2;

  rdelta98 *= 2;
  gdelta98 *= 2;
  bdelta98 *= 2;

  rdelta09 *= 2;
  gdelta09 *= 2;
  bdelta09 *= 2;

  accum88 r88 = pos1color.r << 8;
  accum88 g88 = pos1color.g << 8;
  accum88 b88 = pos1color.b << 8;

  accum88 r99 = pos2color.r << 8;
  accum88 g99 = pos2color.g << 8;
  accum88 b99 = pos2color.b << 8;

  accum88 r00 = pos3color.r << 8;
  accum88 g00 = pos3color.g << 8;
  accum88 b00 = pos3color.b << 8;

  // for( uint16_t i = pos1-1; i > 0; --i) {
  //     leds[i] = CRGB( pos1color.r, pos1color.g, pos1color.b);
  // }

  for (uint16_t i = pos1; i <= pos2; ++i)
  {
    leds[i] = CRGB(r88 >> 8, g88 >> 8, b88 >> 8);
    r88 += rdelta87;
    g88 += gdelta87;
    b88 += bdelta87;
  }

  for (uint16_t i = pos2; i <= pos3; ++i)
  {
    leds[i] = CRGB(r99 >> 8, g99 >> 8, b99 >> 8);
    r99 += rdelta98;
    g99 += gdelta98;
    b99 += bdelta98;
  }

  for (uint16_t i = pos3; i <= pos4; ++i)
  {
    leds[i] = CRGB(r00 >> 8, g00 >> 8, b00 >> 8);
    r00 += rdelta09;
    g00 += gdelta09;
    b00 += bdelta09;
  }

  //    for( uint16_t i = pos4 +1; i <NEOPIXEL_TOTAL; ++i) {
  //     leds[i] = CRGB( pos4color.r, pos4color.g, pos4color.b);
  // }
}

// fetch data from EEPROM
void fetchData(void)
{
  FastLED.clear();

  // sBrightness = prfs.getUInt("Brightness");
  // sAccSync[0] = prfs.getUInt("AccSync");
  // sAccSync[1] = prfs.getUInt("Sensitivity");
  // sMultiColor = prfs.getUInt("MultiColor");
  // sColorPos[0] = prfs.getUInt("Color1Pos");
  // sColor[0].r = prfs.getUInt("Color1R");
  // sColor[0].g = prfs.getUInt("Color1G");
  // sColor[0].b = prfs.getUInt("Color1B");
  // sColorPos[1] = prfs.getUInt("Color2Pos");
  // sColor[1].r = prfs.getUInt("Color2R");
  // sColor[1].g = prfs.getUInt("Color2G");
  // sColor[1].b = prfs.getUInt("Color2B");
  // sColorPos[2] = prfs.getUInt("Color3Pos");
  // sColor[2].r = prfs.getUInt("Color3R");
  // sColor[2].g = prfs.getUInt("Color3G");
  // sColor[2].b = prfs.getUInt("Color3B");
  // sColorPos[3] = prfs.getUInt("Color4Pos");
  // sColor[3].r = prfs.getUInt("Color4R");
  // sColor[3].g = prfs.getUInt("Color4G");
  // sColor[3].b = prfs.getUInt("Color4B");
  // sColorPos[4] = prfs.getUInt("Color5Pos"); // Master Color
  // sColor[4].r = prfs.getUInt("Color5R");
  // sColor[4].g = prfs.getUInt("Color5G");
  // sColor[4].b = prfs.getUInt("Color5B");
  // sColorPos[5] = prfs.getUInt("Color5Pos"); // Welcome Color
  // sColor[5].r = prfs.getUInt("Color6R");
  // sColor[5].g = prfs.getUInt("Color6G");
  // sColor[5].b = prfs.getUInt("Color6B");
  // prfs.end();

  // Serial.println("saved EEPROM : " + String(EEPROM.read(10)));


      mBrightness= prfs.getUInt("brightness");
      mMode = prfs.getUInt("mode");
      sColor[0].r = prfs.getUInt("0R"); // welcome
      sColor[0].g = prfs.getUInt("0G");
      sColor[0].b = prfs.getUInt("0B");
      sColor[1].r = prfs.getUInt("1R"); // single
      sColor[1].g = prfs.getUInt("1G");
      sColor[1].b = prfs.getUInt("1B");
      sColor[2].r = prfs.getUInt("2R"); // grad1
      sColor[2].g = prfs.getUInt("2G");
      sColor[2].b = prfs.getUInt("2B");
      sColor[3].r = prfs.getUInt("3R"); // grad2
      sColor[3].g = prfs.getUInt("3G");
      sColor[3].b = prfs.getUInt("3B");

  for (int i = 0; i < 3; i++)
  {
    Serial.println("fetched  |" +String(i) +   "r : " + String(sColor[i].r) + "g : " + String(sColor[i].g) + "b : " + String(sColor[i].b));
  }

  Serial.println("fetched  |  brightness : " + String(mBrightness) + "mode"+ String(mMode));
}

// show welcome light
void showWelcomeLight(void)
{
  for (int i = 0; i < 255; i++)
  {

    fill_solid(leds, NUM_LEDS, sColor[0]);
    FastLED.setBrightness(i);
    FastLED.show();
    delay(5);
  }
  delay(100);
  for (uint8_t i = 255; i > 0; i--)
  {

    fill_solid(leds, NUM_LEDS, sColor[0]);
    FastLED.setBrightness(i);
    FastLED.show();
    delay(5);
  }
  delay(150);
  FastLED.clear();
}
// shows fetched Color
void showFetchedColor(void)
{
  for (int i = 0; i < mBrightness; i++)
  {

    if(mMode >=2){
      fill_gradient_RGB(leds,NUM_LEDS, sColor[2],sColor[3]);
    }
    else if(mMode ==1){
      fill_solid(leds,NUM_LEDS, sColor[1]);
    }
    FastLED.setBrightness(i);
    FastLED.show();
    delay(10);
  }
}

void setup()
{
  prfs.begin("my-app", false);
  EEPROM.begin(26);
  fetchData(); // fetch data from EEPROM

  FastLED.clear();

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); // Use this for WS2801 or APA102
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);      // This is used by the power management functionality and is currently set at 5V, 500mA.
  FastLED.setBrightness(max_bright);

  Serial.begin(115200);

  Serial.println("Begin Setup BLE Service and Characteristics");

  for (int i = 0; i < 3; i++)
  {
    Serial.println("fetched  |" +String(i) +   "r : " + String(sColor[i].r) + "g : " + String(sColor[i].g) + "b : " + String(sColor[i].b));
  }

  Serial.println("fetched  |  brightness : " + String(mBrightness) + "mode"+ String(mMode));


  showWelcomeLight();
  showFetchedColor(); // show previous color.

  // BT Line
  // Configure thes server
  BLEDevice::init(PERIPHERAL_NAME);
  BLEServer *pServer = BLEDevice::createServer();

  // Create the service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a characteristic for the service
  // BLECharacteristic *pInputChar = pService->createCharacteristic(
  //                             CHARACTERISTIC_INPUT_UUID,
  //                             BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_WRITE);
  pOutputChar = pService->createCharacteristic(
      CHARACTERISTIC_OUTPUT_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  // BLECharacteristic *pColorChar = pService->createCharacteristic(
  //     COLOR_UUID,
  //     BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_WRITE);

  BLECharacteristic *pAccSyncChar = pService->createCharacteristic(
      ACC_SYNC_UUID,
      BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_WRITE);
  BLECharacteristic *pBrightnessChar = pService->createCharacteristic(
      BRIGHTNESS_UUID,
      BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_WRITE);

  BLECharacteristic *pDataMgmtChar = pService->createCharacteristic(
      DATA_MGMT_UUID,
      BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_WRITE);

  BLECharacteristic *pRestartChar = pService->createCharacteristic(
      RESTART_UUID,
      BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_WRITE);

  BLECharacteristic *pMasterChar = pService->createCharacteristic(
      MASTER_UUID,
      BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_WRITE);

  BLECharacteristic *pColorChar = pService->createCharacteristic(
      COLOR_UUID,
      BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_WRITE);

  // Hook callback to report server events
  pServer->setCallbacks(new ServerCallbacks());

  pColorChar->setCallbacks(new ColorCallbacks());
  pAccSyncChar->setCallbacks(new AccSyncCallbacks());
  pBrightnessChar->setCallbacks(new BrightnessCallbacks());
  pDataMgmtChar->setCallbacks(new DataMgmtCallbacks());
  pRestartChar->setCallbacks(new RestartCallbacks());

  pColorChar->setCallbacks(new ColorCallbacks());
  pMasterChar->setCallbacks(new MasterCallbacks());

  // Initial characteristic value
  outputData[0] = 0x00;
  outputData[0] = 0x00;
  outputData[0] = 0x00;
  outputData[0] = 0x01;
  pOutputChar->setValue((uint8_t *)outputData, 1);
  // Start the service
  pService->start();

  // Advertise the service
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE Service is advertising");
  //     FastLED.clear();
  //       gradRGB(leds, 0, CRGB(255,204,112), 32, CRGB(200,80,192), 16, CRGB(200,80,192), 33, CRGB(65,88,208));
  //   FastLED.setBrightness(125);

  // FastLED.show();
}

void loop()
{
  delay(10);
}
