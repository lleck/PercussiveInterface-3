
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
#define CLOCK_PIN 47
#define MOSI_PIN 48
#define MISO_PIN 14
#define MUX1_SYNC_PIN 1
#define MUX2_SYNC_PIN 9
#define TMAG_CS_PIN 21
#define IR_VIBROMETER_PIN 13 // QRE 1113
#define IR_SENSOR_PIN 6      // Rotation Interrupt
// #define IO35 35

// Globals
const uint8_t pixelCount = 52;
const uint8_t sensorCount = 26;
const uint16_t angularDivisions = 360;
// currently active sensor (used for debugging)
uint8_t currentSensor = 0;
// Zähler für die aktuelle angulare Division
uint16_t numDiv = 0;

// two 2D array buffers for the magnetic sensors (left & right arm)
int magneticArray1[sensorCount*2][angularDivisions] ;
int magneticArray2[sensorCount*2][angularDivisions] ;

// Channel select commands for ADG731 Mux (center first)
uint8_t mux1_ch[26] = {30, 31, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 17, 18, 19, 20, 21, 22, 23};
uint8_t mux2_ch[26] = {23, 22, 21, 20, 19, 18, 17, 16, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 12, 13, 14, 15, 31, 30};

uint8_t led_ch[52] = {51, 0, 50, 1, 49, 2, 48, 3, 47, 4, 46, 5, 45, 6, 44, 7, 43, 8, 42, 9, 41, 10, 40, 11, 39, 12, 38, 13, 37, 14, 36, 15, 35, 16, 34,
                      17, 33, 18, 32, 19, 31, 20, 30, 21, 29, 22, 28, 23, 27, 24, 26, 25};
// init the TMAG5170 Class
TMAG5170 magneticSensor;
// init the NeoPixelBus instance with Spi and alternate Pins
NeoPixelBus<DotStarBgrFeature, DotStarMethod> strip(pixelCount, LED_CLOCK_PIN, LED_DATA_PIN);

uint8_t colorSaturation = 5;
RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

// forward declaring functions as this is not written in Arduino IDE
void sensorChannel(uint8_t muxNr, uint8_t channelNr);
void mux_off(int muxNr);
void readPosition();
int readMagneticSensor();

SPISettings muxSPI(1000, MSBFIRST, SPI_MODE2); // spi configuration for the ADG731

void setup()
{
  D_SerialBegin(9600);
  delay(2000);
  SPI.begin(CLOCK_PIN, MISO_PIN, MOSI_PIN);
  pinMode(MUX1_SYNC_PIN, OUTPUT); // set the SS pin as an output
  pinMode(MUX2_SYNC_PIN, OUTPUT); // set the SS pin as an output
  pinMode(TMAG_CS_PIN, OUTPUT);
  pinMode(IR_SENSOR_PIN, INPUT);
  digitalWrite(MUX1_SYNC_PIN, HIGH);
  digitalWrite(MUX2_SYNC_PIN, HIGH);
  digitalWrite(TMAG_CS_PIN, HIGH);

  bool error = false;
  // start the strip on the defined SPI bus and init to black = all pixels off
  strip.Begin();
  strip.ClearTo(black); // this resets all the DotStars to an off state
  strip.Show();

  magneticSensor.begin(TMAG_CS_PIN);
  magneticSensor.disable_crc();

  for (int i = 0; i < sensorCount && !error; i++)
  {
    sensorChannel(1, i);
    delay(1);
    magneticSensor.default_cfg(&error);
    delay(1);
    strip.SetPixelColor(led_ch[i], red);
    strip.Show();
    delay(50);
    strip.SetPixelColor(led_ch[i], black);
    strip.Show();
  }
  mux_off(1);
  strip.ClearTo(black); // this resets all the DotStars to an off state
  strip.Show();

  for (int j = 0; j < sensorCount && !error; j++)
  {
    sensorChannel(2, j);
    delay(1);
    magneticSensor.default_cfg(&error);
    delay(1);
    strip.SetPixelColor(led_ch[j + sensorCount], red);
    strip.Show();
    delay(50);
    strip.SetPixelColor(led_ch[j + sensorCount], black);
    strip.Show();
  }
  mux_off(2);

  strip.ClearTo(black); // this resets all the DotStars to an off state
  strip.Show();

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
    SPI.endTransaction();
    break;
  case 2:
    SPI.beginTransaction(muxSPI);
    digitalWrite(MUX2_SYNC_PIN, LOW);
    SPI.transfer(mux2_ch[channelNr]); // send a command to select channel
    digitalWrite(MUX2_SYNC_PIN, HIGH);
    SPI.endTransaction();
    break;
  default:
    break;
  }
}

void mux_off(int muxNr)
{
  SPI.beginTransaction(muxSPI);
  if (muxNr == 1)
  {
    digitalWrite(MUX1_SYNC_PIN, LOW);
    SPI.transfer(0x80);
    digitalWrite(MUX1_SYNC_PIN, HIGH);
  }
  else if (muxNr == 2)
  {
    digitalWrite(MUX2_SYNC_PIN, LOW);
    SPI.transfer(0x80);
    digitalWrite(MUX2_SYNC_PIN, HIGH);
  }
  SPI.endTransaction();
}

void readPosition()
{
  for (int sensorIndex = 0; sensorIndex < 51; sensorIndex++)
  {
    //bei geraden Indexzahlen
    if (sensorIndex % 2 == 0)
    {
      sensorChannel(1, sensorIndex/2);
      // Read from the SPI-controlled sensor
      int sensorValue1 = readMagneticSensor();
      D_print(sensorValue1);
      D_print("\t");
      // Store the sensor value in the array
      magneticArray1[sensorIndex][numDiv] = sensorValue1;
      mux_off(1);

      // Select the appropriate channel on the second multiplexer
      sensorChannel(2, sensorIndex/2);
      // Read from the SPI-controlled sensor
      int sensorValue2 = readMagneticSensor();
      D_println(sensorValue2);
      // Store the sensor value in the array
      magneticArray2[sensorIndex][numDiv] = sensorValue2;
      mux_off(2);

       
    }
   
    //bei ungeraden Indexzahlen
    else
    {
      magneticArray1[sensorIndex][numDiv] = (magneticArray1[sensorIndex+1][numDiv] + magneticArray1[sensorIndex-1][numDiv]) / 2;
      magneticArray2[sensorIndex][numDiv] = (magneticArray2[sensorIndex+1][numDiv] + magneticArray2[sensorIndex-1][numDiv]) / 2;
    }
 

    // // Print the sensor index and values
    // D_print("Sensor ");
    // D_print(sensorIndex + 1);  // Sensor index starts from 1
    // D_print(": ");
    // D_print("MUX1 - ");
    // D_print(sensorValue1);
    // D_print(", MUX2 - ");
    // D_println(sensorValue2);
  }
  // numDiv++;
  // if (numDiv >= angularDivisions)
  numDiv = 0;
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
   int sensorValue1 = analogRead(IR_SENSOR_PIN);
   D_println(sensorValue1);
   delay(200);
  // // u_long start_time = micros();
   readPosition();
  // // u_long end_time = micros();
  // // u_long duration = end_time - start_time;
  // // D_println(duration);
  //  delay(50);

  // for (int j = 0; j < sensorCount*2; j++)
  // {
  //   if (magneticArray1[j][0] > 200)
  //   {
  //     colorSaturation = map(magneticArray1[j][0], 0, 32768, 0, 40);
  //     strip.SetPixelColor(led_ch[j], RgbColor(0, colorSaturation, 0));
  //   }
  //   else
  //   {
  //     strip.SetPixelColor(led_ch[j], black);
  //   }
  //   strip.Show();
  // }
  // // int rotVal = analogRead(IR_SENSOR_PIN);
  // // D_println(rotVal);
}
