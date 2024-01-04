
#define DEBUG 1 // 0 to disable serial prints

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
uint8_t ch_select_cmd[34] = {0x80,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x40};

// init the TMAG5170 Class
TMAG5170 magneticSensor;
// init the NeoPixelBus instance with Spi and alternate Pins
NeoPixelBus<DotStarBgrFeature, DotStarSpi40MhzMethod> strip(pixelCount);

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);       
RgbColor white(colorSaturation);
RgbColor black(0);

//forward declaring functions as this is not written in Arduino IDE
void sensorChannel (uint8_t muxNr, uint8_t channelNr);
void readPosition();
float readMagneticSensor();

SPISettings muxSPI(1000000, MSBFIRST, SPI_MODE2); // spi configuration for the ADG731



void setup(){
  D_SerialBegin(9600);
  SPI.begin(CLOCK_PIN, MISO_PIN, MOSI_PIN);
  pinMode(MUX1_SYNC_PIN, OUTPUT); // set the SS pin as an output
  pinMode(MUX2_SYNC_PIN, OUTPUT); // set the SS pin as an output
  pinMode(TMAG_CS_PIN, OUTPUT);
  digitalWrite(MUX1_SYNC_PIN, HIGH);
  digitalWrite(MUX2_SYNC_PIN, HIGH);
  digitalWrite(TMAG_CS_PIN, HIGH);        

  bool error = false;
  //  // start the strip on the defined SPI bus and init to black = all pixels off
  // strip.Begin(LED_CLOCK_PIN, LED_DATA_PIN, LED_DATA_PIN, LED_CS_PIN);
  // strip.ClearTo(black); // this resets all the DotStars to an off state
  // strip.Show();



  magneticSensor.begin(TMAG_CS_PIN);
  magneticSensor.disable_crc();

  for (int i = 1; i <= sensorCount && !error; i++)
  {
    sensorChannel(1, i);
    magneticSensor.default_cfg(&error);
  }
  
  for (int j = 1; j <= sensorCount && !error; j++)
  {
    sensorChannel(2, j);
    magneticSensor.default_cfg(&error);  
  }

  if (error)
  {
    D_print("error conf. sensor nr. "); // Error check
    D_println(currentSensor);
  }

  // send a command to set all Channels Off
  sensorChannel(1, 0);
  sensorChannel(2, 0);

  D_println("TMAG_config_complete");

}

void sensorChannel (uint8_t muxNr, uint8_t channelNr) 
{   
  switch (muxNr)
  {
    case 1:
      SPI.beginTransaction(muxSPI);
      digitalWrite(MUX1_SYNC_PIN, LOW);
      SPI.transfer(ch_select_cmd[channelNr]);      // send a command to select channel 
      currentSensor = channelNr;                   // keep track of current sensor for debugging
      digitalWrite(MUX1_SYNC_PIN, HIGH);
      SPI.endTransaction();
    break;
    case 2:
      SPI.beginTransaction(muxSPI);
      digitalWrite(MUX2_SYNC_PIN, LOW);
      SPI.transfer(ch_select_cmd[channelNr]);      // send a command to select channel 
      currentSensor = channelNr + sensorCount;     // keep track of current sensor for debugging
      digitalWrite(MUX2_SYNC_PIN, HIGH);
      SPI.endTransaction();
    break;
    default: break;
    
  }
}

void readPosition()
{
  for (int sensorIndex = 0; sensorIndex < sensorCount; sensorIndex++)
  {
    // Select the appropriate channel on the first multiplexer
    sensorChannel(1, sensorIndex);
    // Read from the SPI-controlled sensor
    float sensorValue1 = readMagneticSensor();
    // Store the sensor value in the array
    magneticArray1[sensorIndex][numDiv] = sensorValue1;
    sensorChannel(1, 0);
    
    // Select the appropriate channel on the second multiplexer
    sensorChannel(2, sensorIndex);
    // Read from the SPI-controlled sensor
    float sensorValue2 = readMagneticSensor();
    // Store the sensor value in the array
    magneticArray2[sensorIndex][numDiv] = sensorValue2;
    sensorChannel(2, 0);
    // // Print the sensor index and values
    D_print("Sensor ");
    D_print(sensorIndex + 1);  // Sensor index starts from 1
    D_print(": ");
    D_print("MUX1 - ");
    D_print(sensorValue1);
    D_print(", MUX2 - ");
    D_println(sensorValue2);
  }
  // numDiv++;
  // if (numDiv >= angularDivisions)
  //   numDiv = 0;
}

float readMagneticSensor()
{
  bool error = false;
  float value = magneticSensor.getZresult(&error);
  if (error)
  {
   D_print("error reading sensor nr. ");
   D_println(currentSensor);
   delay(10);
  }
  return value;
}

void loop(){
    
    // readPosition();
     //magneticSensor.simple_read(TMAG5170_REG_CONV_STATUS);
    // // Select the channel on the first multiplexer

     sensorChannel(2, 4);
     D_println("mux2 ch 4");
     float sensorValue2 = readMagneticSensor();
     D_println(sensorValue2);
     delay(1000);
     sensorChannel(2, 0);
     D_println("mux2 off");
     delay(200);
     sensorChannel(1, 4);
     D_println("mux1 ch 4");
     float sensorValue1 = readMagneticSensor();
     D_println(sensorValue1);
     delay(1000);
     sensorChannel(1, 0 );
     D_println("mux1 off");
     delay(200);

}

