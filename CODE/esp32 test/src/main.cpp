#include <Arduino.h>
#include <ADG731.h>
#include <myTMAG5170.h>
#include <NeoPixelBus.h>

#define LED_CLOCK_PIN 12
#define LED_DATA_PIN 11
#define LED_CS_PIN -1
#define colorSaturation 128
#define CLOCK_PIN 47
#define MOSI_PIN 48
#define MISO_PIN 14
#define MUX1_SYNC_PIN 1
#define MUX2_SYNC_PIN 9
#define TMAG_CS_PIN 21
#define IR_VIBROMETER_PIN 13

// Globals
const uint8_t pixelCount = 52;
const uint8_t sensorCount = 26;
const uint16_t angularDivisions = 360;
// currently active sensor (used for debugging)
uint8_t currentSensor = 0;
// Zähler für die aktuelle angulare Division
uint16_t numDiv = 0;
// two 2D array buffers for the magnetic sensors (left & right arm)
float magneticArray1[sensorCount][angularDivisions];
float magneticArray2[sensorCount][angularDivisions];

// init two instances of the adg731 32ch mux
ADG731 mux1(CLOCK_PIN, MOSI_PIN, MUX1_SYNC_PIN);
ADG731 mux2(CLOCK_PIN, MOSI_PIN, MUX2_SYNC_PIN);

// init the TMAG5170 Class
TMAG5170 magneticSensor;
// init the NeoPixelBus instance with Spi and alternate Pins
NeoPixelBus<DotStarBgrFeature, DotStarEsp32DmaSpi3Method> strip(pixelCount);

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

void setup(){
  Serial.begin(9600);
  delay(1000);
  pinMode(TMAG_CS_PIN,OUTPUT);
  magneticSensor.begin(TMAG_CS_PIN);
  bool error = false;
   // start the strip on the defined SPI bus and init to black = all pixels off
  strip.Begin(LED_CLOCK_PIN, LED_DATA_PIN, LED_DATA_PIN, LED_CS_PIN);
  strip.ClearTo(black); // this resets all the DotStars to an off state
  strip.Show();
  
  for (int i = 0; i < sensorCount && !error; i++)
  {
    mux1.setChannel(i);
    currentSensor = i;
    digitalWrite(TMAG_CS_PIN, LOW);
    magneticSensor.default_cfg(&error);
      if (error)
      {
        Serial.print("error conf. sensor nr. "); // Error check 
        Serial.println(currentSensor);
        return;
      }
    digitalWrite(TMAG_CS_PIN, HIGH);
  }
  
  for (int j = 0; j < sensorCount && !error; j++)
  {
    mux2.setChannel(j);
    currentSensor = j + sensorCount;
    digitalWrite(TMAG_CS_PIN, LOW);
    magneticSensor.default_cfg(&error);
    if (error)
      {
        Serial.print("error conf. sensor nr. "); // Error check
        Serial.println(currentSensor);
        return;
      }
    digitalWrite(TMAG_CS_PIN, HIGH);
  }
  mux1.allOff();
  mux2.allOff();
  Serial.println("TMAG_config_complete");
}

float readMagneticSensor()
{
  bool error = false;
  float value = magneticSensor.getZresult(&error);
  if (error)
  {
   Serial.print("error reading sensor nr. ");
   Serial.println(currentSensor);
   delay(10);
  }
  return value;
}

void loop(){
    // Select the appropriate channel on the first multiplexer
    digitalWrite(TMAG_CS_PIN, LOW);
    mux1.setChannel(30);
    currentSensor = 30;
    delay(1000);
    // Read from the SPI-controlled sensor
    float sensorValue1 = readMagneticSensor();
    Serial.println(sensorValue1);
    delay(1000);
    digitalWrite(TMAG_CS_PIN, HIGH);
    //mux1.allOff();
  
}
