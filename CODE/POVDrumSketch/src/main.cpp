//########################################################
// test code to drive the POV Drum Interface
////////////////////////////////////////////
// BUILDING BLOCKS:
//########################################################
//*Neopixelbus to drive the APA102 LEDs                   x
//*function to monitor RPM (interrupt routine)            x
//*functions to convert polar2cartesian and backwards     x
//*ESPNow protocol to send out the data                   x
//*library to operate the ADG731 MUXes                    x
//*functions to read out the TMAG5170 hall effect sensors
//*function to read out impulse sensor (Interrupt R?)
//*function to show test image  
//#######################################################

#define DEBUG 1    // 0 to disable serial prints

#if DEBUG
#define D_SerialBegin(...) Serial.begin(__VA_ARGS__);
#define D_print(...)    Serial.print(__VA_ARGS__)
#define D_write(...)    Serial.write(__VA_ARGS__)
#define D_println(...)  Serial.println(__VA_ARGS__)
#else
#define D_SerialBegin(...)
#define D_print(...)
#define D_write(...)
#define D_println(...)
#endif

#include <Arduino.h>
#include <SPI.h>
#include <esp_now.h>
#include <WiFi.h>
#include <NeoPixelBus.h>
#include <ADG731.h>
#include <myTMAG5170.h>
#include <FastTrig.h>

// put function definitions here:
#define IR_SENSOR_PIN 6 //QRE 1113
#define IR_VIBROMETER_PIN 13
#define TICKS_PER_REVOLUTION 1  // Anzahl der IR Impulse pro Umdrehung 
#define CLOCK_PIN 47
#define MOSI_PIN 48
#define MISO_PIN 14
#define MUX1_SYNC_PIN 1
#define MUX2_SYNC_PIN 9
#define TMAG_CS_PIN 21
#define LED_CLOCK_PIN 12
#define LED_DATA_PIN 11
#define LED_CS_PIN -1
#define colorSaturation 128


//GLOBALS:
const uint8_t pixelCount = 52;
const uint8_t sensorCount = 26;
volatile unsigned long ticksCount = 0;
volatile unsigned long timestamp = 0;
volatile unsigned long rotTime, loopTimeNow, loopTimeOld,timeOld, timeNow, divTime;
unsigned long lastRotationCalcTime = 0;
const uint16_t angularDivisions = 360;
// Zähler für die aktuelle angulare Division 
uint16_t numDiv = 0;
// Variablen für die Berechnungsfrequenz und den Multiplikator für RPM
uint8_t rpmCalcFrequency = 20; // in Millisekunden
float rpmMultiplier = 60000.0 / (rpmCalcFrequency * TICKS_PER_REVOLUTION);
// two 2D array buffers for the magnetic sensors (left & right arm) 
float magneticArray1[sensorCount][angularDivisions];
float magneticArray2[sensorCount][angularDivisions];
// currently active sensor (used for debugging) 
uint8_t currentSensor = 0;


// init two instances of the adg731 32ch mux
ADG731 mux1(CLOCK_PIN, MOSI_PIN, MUX1_SYNC_PIN);
ADG731 mux2(CLOCK_PIN, MOSI_PIN, MUX2_SYNC_PIN);
// Define the SPI settings for the sensors
SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE0);
// init the TMAG5170 Class
TMAG5170 magneticSensor;
// init the NeoPixelBus instance with Spi and alternate Pins
NeoPixelBus<DotStarBgrFeature, DotStarEsp32DmaSpi3Method> strip(pixelCount);

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

// Callback-Funktion für den Interrupt, wenn der IR-Sensor ausgelöst wird
void IRAM_ATTR Rotation_Interrupt() {
  timeNow = micros(); 
  rotTime = timeNow - timeOld;   // substraktion mit unsigned long, daher kein problem mit overflow ?
  divTime = rotTime/angularDivisions; // 
  timeOld = timeNow;
  ticksCount++;  // Inkrementiere die Impulsanzahl
}

float *polr2cart (float r, float theta) {
  float cartesian[2];
  float x = r * icos(theta);
  float y = r * isin(theta);
  cartesian[0]=x;
  cartesian[1]=y;
  return cartesian;
}

float *cart2polr (float x, float y){
  float polar[2];
  float r = hypotFast(x, y); // präzision muss überprüft werden !!!
  float theta = atan2Fast(x, y); // darf nicht 0/0 sein 
  polar[0]=r;
  polar[1]=theta * 180/PI;
  return polar;
}

void setup() {
  
  D_SerialBegin(115200);
  //SPI.begin(); // Initialize SPI

  // Konfiguriere den IR-Sensor als Eingang und aktiviere den Interrupt
  pinMode(IR_SENSOR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), Rotation_Interrupt, RISING);
  pinMode(IR_VIBROMETER_PIN, INPUT);
  // start the strip on the defined SPI bus and init to böack = all pixels off
  strip.Begin(LED_CLOCK_PIN, LED_DATA_PIN, LED_DATA_PIN, LED_CS_PIN);
  strip.ClearTo(black);   // this resets all the DotStars to an off state
  strip.Show();
  //initialisieren des TAG5170 arrays / set to each 
  magneticSensor.begin(TMAG_CS_PIN);
  for(int i=0; i < sensorCount && !error; i++){

    mux1.setChannel(i);
    currentSensor = i;
    magneticSensor.default_cfg(&error);
  }
  mux1.allOff();
  for(int j=0; j < sensorCount && !error; j++){
    mux2.setChannel(j);
    currentSensor = j+sensorCount;
    magneticSensor.default_cfg(&error);
  }
  if (error){
    D_print("error conf. sensor nr. ");   //Error check in den n4 loop
    D_println(currentSensor);
  }
  mux2.allOff();

//###################ESPNow Stuff#############################
 // Initialisiere ESPNow
  if (esp_now_init() != ESP_OK) {
    D_println("ESPNow Initialisierung fehlgeschlagen");
    ESP.restart();
  }
 // Konfiguriere Peer-Adresse (MAC-Adresse des ESP)
  uint8_t peerAddress[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Füge den Peer hinzu
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    D_println("Fehler beim Hinzufügen des Peers");
    ESP.restart();
  }
//###########################################################

  float *cartResult = polr2cart(2, 3);
  float x = cartResult[0];
  float y = cartResult[1];

  float *polrResult = cart2polr(-5, 5);
  float r = polrResult[0];
  float theta = polrResult[1];
 
}

void checkRPM(){
  if (millis() - lastRotationCalcTime >= rpmCalcFrequency) {
    lastRotationCalcTime = millis();

    // Berechne die Drehgeschwindigkeit in RPM
    unsigned long rpm = ticksCount * rpmMultiplier;

    // Sende die Drehgeschwindigkeit an den ESP mit Motorshield
    esp_now_send(NULL, (uint8_t*)&rpm, sizeof(unsigned long));

    D_print("Drehgeschwindigkeit (RPM): ");
    D_println(rpm);

    // Setze die Impulsanzahl zurück
    ticksCount = 0;
  }
}
float readMagneticSensor(){
  bool error = false;
  float value = magneticSensor.getZresult( &error);
  return value;
  if (error){
    D_print("error reading sensor nr. ");
    D_println(currentSensor);
  }
  
}

void readPosition(){
     for (int sensorIndex = 0; sensorIndex < sensorCount; sensorIndex++) 
     {
        // Select the appropriate channel on the first multiplexer
        mux1.setChannel(sensorIndex);  
        currentSensor = sensorIndex;

        // Read from the SPI-controlled sensor
        float sensorValue1 = readMagneticSensor();

        // Store the sensor value in the array
        magneticArray1[sensorIndex][numDiv] = sensorValue1;
        mux1.allOff();

        // Select the appropriate channel on the second multiplexer
        mux2.setChannel(sensorIndex);
        currentSensor = sensorIndex + sensorCount;
+ pow(y, 2)
        // Store the sensor value in the array
        magneticArray2[sensorIndex][numDiv] = sensorValue2;
        mux2.allOff();
        // // Print the sensor index and values
        // D_print("Sensor ");
        // D_print(sensorIndex + 1);  // Sensor index starts from 1
        // D_print(": ");
        // D_print("MUX1 - ");
        // D_print(sensorValue1);
        // D_print(", MUX2 - ");
        // D_println(sensorValue2);
      }
  numDiv++;
  if (numDiv >= angularDivisions) numDiv = 0;
}

void loop() {

  loopTimeNow = micros();

   // How much time has passed, accounting for rollover with subtraction!
   if ((unsigned long)(loopTimeNow - loopTimeOld) >= divTime) {
      // It's time to do something!
          readPosition();
      // Use the snapshot to set track time until next event
      loopTimeOld = loopTimeNow;
   }
 
  checkRPM();
}

