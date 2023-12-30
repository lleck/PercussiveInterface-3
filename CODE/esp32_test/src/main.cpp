#include <Arduino.h>
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

// Channel select commands for ADG731 Mux
uint8_t ch_select_cmd[34] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x40,0x80};

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
  SPI.begin(CLOCK_PIN, MISO_PIN, MOSI_PIN);
  pinMode(MUX1_SYNC_PIN, OUTPUT); // set the SS pin as an output
  pinMode(MUX2_SYNC_PIN, OUTPUT); // set the SS pin as an output
  pinMode(TMAG_CS_PIN, OUTPUT);
  digitalWrite(MUX1_SYNC_PIN, HIGH);
  digitalWrite(MUX2_SYNC_PIN, HIGH);
  digitalWrite(TMAG_CS_PIN, HIGH);        

  bool error = false;
   // start the strip on the defined SPI bus and init to black = all pixels off
  strip.Begin(LED_CLOCK_PIN, LED_DATA_PIN, LED_DATA_PIN, LED_CS_PIN);
  strip.ClearTo(black); // this resets all the DotStars to an off state
  strip.Show();

  magneticSensor.begin(TMAG_CS_PIN);
  magneticSensor.disable_crc();

  for (int i = 0; i < sensorCount && !error; i++)
  {
    digitalWrite(MUX1_SYNC_PIN, LOW);
    SPI.transfer(ch_select_cmd[i]);      // send a command to select channel 
    digitalWrite(MUX1_SYNC_PIN, HIGH);
    currentSensor = i;
    magneticSensor.default_cfg(&error);
  }
  
  for (int j = 0; j < sensorCount && !error; j++)
  {
    digitalWrite(MUX2_SYNC_PIN, LOW);
    SPI.transfer(ch_select_cmd[j]);      // send a command to select channel 31
    digitalWrite(MUX2_SYNC_PIN, HIGH);
    currentSensor = j + sensorCount;
    magneticSensor.default_cfg(&error);   
  }
  if (error)
      {
        Serial.print("error conf. sensor nr. "); // Error check
        Serial.println(currentSensor);
      }
    digitalWrite(MUX1_SYNC_PIN, LOW);
    SPI.transfer(ch_select_cmd[34]);      // send a command to set all Off
    digitalWrite(MUX1_SYNC_PIN, HIGH);
    digitalWrite(MUX2_SYNC_PIN, LOW);
    SPI.transfer(ch_select_cmd[34]);      // send a command to set all Off
    digitalWrite(MUX2_SYNC_PIN, HIGH);

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
   
    
    digitalWrite(MUX1_SYNC_PIN, LOW);
    SPI.transfer(ch_select_cmd[30]);      // send a command to select channel 31
    digitalWrite(MUX1_SYNC_PIN, HIGH);



    //magneticSensor.simple_read(TMAG5170_REG_CONV_STATUS);
    //Read from the SPI-controlled sensor
    float sensorValue = readMagneticSensor();
    Serial.println(sensorValue);
    delay(10);



}

