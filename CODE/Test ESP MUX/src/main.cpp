
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

const uint8_t sensorCount = 26;

// init the TMAG5170 Class
TMAG5170 magneticSensor;

// Channel select commands for ADG731 Mux
uint8_t mux1_ch[26] = {30, 31, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 17, 18, 19, 20, 21, 22, 23};
uint8_t mux2_ch[26] = {23, 22, 21, 20, 19, 18, 17, 16, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 12, 13, 14, 15, 31, 30};

SPISettings  muxSPI(1000000, MSBFIRST, SPI_MODE2); // spi configuration for the ADG731
SPISettings tmagSPI(1000000, MSBFIRST, SPI_MODE0);

void mux_off(int muxNr);

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
float readMagneticSensor()
{
  bool error = false;
  float value = magneticSensor.getZresult(&error);
  if (error)
  {
    D_print("error reading sensor ");
    delay(10);
  }
  return value;
}

void setup()
{
  D_SerialBegin(115200);
  bool error = 0;
  delay(2000);
  
  SPI.begin(CLOCK_PIN, MISO_PIN, MOSI_PIN);
  pinMode(MUX1_SYNC_PIN, OUTPUT); // set the SS pin as an output
  pinMode(MUX2_SYNC_PIN, OUTPUT); // set the SS pin as an output
 
  pinMode(TMAG_CS_PIN, OUTPUT);
  digitalWrite(MUX1_SYNC_PIN, HIGH);
  digitalWrite(MUX2_SYNC_PIN, HIGH);
  digitalWrite(TMAG_CS_PIN, HIGH);
  
  //magneticSensor.begin(TMAG_CS_PIN);
    //mux_off(1);
    sensorChannel(2, 0);
    delay(50);
    SPI.beginTransaction(tmagSPI);
    delay(5);
    // trash transfer um die clock polarity zu switchen
    SPI.transfer(0x0);
    digitalWrite(TMAG_CS_PIN,LOW);
    SPI.transfer(0x0F);
    SPI.transfer(0x00);
    SPI.transfer(0x04);
    SPI.transfer(0x07);
    digitalWrite(TMAG_CS_PIN,HIGH);
    SPI.endTransaction();
    delay(10);
//read test config
    SPI.beginTransaction(tmagSPI);
    digitalWrite(TMAG_CS_PIN,LOW);
    uint8_t v0 = SPI.transfer(0x8F);
    uint8_t v1 = SPI.transfer(0x00);
    uint8_t v2 = SPI.transfer(0x00);
    uint8_t v3 = SPI.transfer(0x00);
    digitalWrite(TMAG_CS_PIN,HIGH);
    SPI.endTransaction();

  D_println(v0,BIN);
  D_println(v1,BIN);
  D_println(v2,BIN);
  D_println(v3,BIN);
    

    // magneticSensor.disable_crc();
    // magneticSensor.default_cfg(&error);
    
    // magneticSensor.simple_read(0x0f);
  
  // for (int j = 0; j < 26 && !error; j++)
  // {
  //   sensorChannel(2, j);
  //   magneticSensor.disable_crc();
  //   magneticSensor.default_cfg(&error);
  // }
  // mux_off(2);
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
  else
  {
    digitalWrite(MUX2_SYNC_PIN, LOW);
    SPI.transfer(0x80);
    digitalWrite(MUX2_SYNC_PIN, HIGH);
  }
  SPI.endTransaction();
}

void loop()
{
  // float val = readMagneticSensor();
  // D_println(val);
  // // digitalWrite(TMAG_CS_PIN,LOW);
  // // sensorChannel(1,0);
  // // sensorChannel(2,0);
  // // delay(500);
  // // mux_off(2);
  //  delay(500);

}
