
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
int magneticArray1[sensorCount][angularDivisions];
int magneticArray2[sensorCount][angularDivisions];

// Channel select commands for ADG731 Mux
// Channel select commands for ADG731 Mux (center first)
uint8_t mux1_ch[27] = {128, 30, 31, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 17, 18, 19, 20, 21, 22, 23};
uint8_t mux2_ch[27] = {128, 23, 22, 21, 20, 19, 18, 17, 16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 31, 30};

// init the TMAG5170 Class
TMAG5170 magneticSensor;
// init the NeoPixelBus instance with Spi and alternate Pins
// NeoPixelBus<DotStarBgrFeature, DotStarSpi40MhzMethod> strip(pixelCount);

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

// forward declaring functions as this is not written in Arduino IDE
void sensorChannel(uint8_t muxNr, uint8_t channelNr);
void readPosition();
int readMagneticSensor();

SPISettings muxSPI(1000000, MSBFIRST, SPI_MODE2); // spi configuration for the ADG731

void setup()
{

  D_SerialBegin(115200);
  delay(2000);

  SPI.begin(CLOCK_PIN, MISO_PIN, MOSI_PIN);
  pinMode(MUX1_SYNC_PIN, OUTPUT); // set the SS pin as an output
  pinMode(MUX2_SYNC_PIN, OUTPUT); // set the SS pin as an output

  digitalWrite(MUX1_SYNC_PIN, HIGH);
  digitalWrite(MUX2_SYNC_PIN, HIGH);

  magneticSensor.begin(TMAG_CS_PIN);

  bool error = false;

  //  // start the strip on the defined SPI bus and init to black = all pixels off
  // strip.Begin(LED_CLOCK_PIN, LED_DATA_PIN, LED_DATA_PIN, LED_CS_PIN);
  // strip.ClearTo(black); // this resets all the DotStars to an off state
  // strip.Show();

  for (int i = 1; i <= sensorCount && !error; i++)
  {
    sensorChannel(1, i);
    delay(1);
    magneticSensor.disable_crc();
    delay(1);
    magneticSensor.default_cfg(&error);
    delay(1);
  }
  sensorChannel(1, 0);

  for (int j = 1; j <= sensorCount && !error; j++)
  {
    sensorChannel(2, j);
    delay(1);
    magneticSensor.disable_crc();
    delay(1);
    magneticSensor.default_cfg(&error);
    delay(1);
  }
  sensorChannel(2, 0);

  if (error)
  {
    D_print("error conf. sensor nr. "); // Error check
    D_println(currentSensor);
  }

  D_println("TMAG_config_complete");
}

void sensorChannel(uint8_t muxNr, uint8_t channelNr)
{
  switch (muxNr)
  {
  case 1:
    SPI.beginTransaction(muxSPI);
    digitalWrite(MUX1_SYNC_PIN, LOW);
    SPI.transfer(mux1_ch[channelNr]); // send a command to select channel
    digitalWrite(MUX1_SYNC_PIN, HIGH);
    currentSensor = channelNr; // keep track of current sensor for debugging
    SPI.endTransaction();
    digitalWrite(MUX2_SYNC_PIN, LOW);
    digitalWrite(MUX2_SYNC_PIN, HIGH);
    break;
  case 2:
    SPI.beginTransaction(muxSPI);
    digitalWrite(MUX2_SYNC_PIN, LOW);
    SPI.transfer(mux2_ch[channelNr]); // send a command to select channel
    digitalWrite(MUX2_SYNC_PIN, HIGH);
    currentSensor = channelNr + sensorCount; // keep track of current sensor for debugging
    SPI.endTransaction();
    digitalWrite(MUX1_SYNC_PIN, LOW);
    digitalWrite(MUX1_SYNC_PIN, HIGH);
    break;
  default:
    break;
  }
}

void readPosition()
{
  for (int sensorIndex = 1; sensorIndex <= sensorCount; sensorIndex++)
  {
    // Select the appropriate channel on the first multiplexer
    sensorChannel(1, sensorIndex);
    // Read from the SPI-controlled sensor
    int sensorValue1 = readMagneticSensor();
    // Store the sensor value in the array
    magneticArray1[sensorIndex-1][numDiv] = sensorValue1;
    sensorChannel(1, 0);

    // Select the appropriate channel on the second multiplexer
    sensorChannel(2, sensorIndex);
    // Read from the SPI-controlled sensor
    int sensorValue2 = readMagneticSensor();
    // Store the sensor value in the array
    magneticArray2[sensorIndex-1][numDiv] = sensorValue2;
    sensorChannel(2, 0);
    // // Print the sensor index and values
    D_print("Sensor ");
    D_print(sensorIndex); // Sensor index starts from 1
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

int readMagneticSensor()
{
  bool error = false;
  int value = magneticSensor.getZresult(&error);
  if (error)
  {
    D_print("error reading sensor nr. ");
    D_println(currentSensor);
    delay(10);
  }
  return value;
}

void loop()
{
   readPosition();

  // sensorChannel(1, 1);
  // D_println("mux1 ");
  // int sensorValue1 = readMagneticSensor();
  // D_print(sensorValue1);
  // D_print("  \ ");
  // sensorChannel(1, 2);
  // int sensorValue2 = readMagneticSensor();
  // D_print(sensorValue2);
  // D_print("  \ ");
  // sensorChannel(1, 3);
  // int sensorValue3 = readMagneticSensor();
  // D_println(sensorValue3);
  // delay(500);

  // sensorChannel(1, 0);

  // sensorChannel(2, 1);
  // D_println("mux2");
  // int sensorValue4 = readMagneticSensor();
  // D_print(sensorValue4);
  // D_print("  \ ");
  // sensorChannel(2, 2);
  // int sensorValue5 = readMagneticSensor();
  // D_print(sensorValue5);
  // D_print("  \ ");
  // sensorChannel(2, 3);
  // int sensorValue6 = readMagneticSensor();
  // D_println(sensorValue6);
   delay(100);

  // sensorChannel(2, 0);

}
