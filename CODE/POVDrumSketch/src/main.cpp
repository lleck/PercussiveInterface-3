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

#include <Arduino.h>
#include <SPI.h>
#include <esp_now.h>
#include <WiFi.h>
#include <NeoPixelBus.h>
#include <ADG731.h>

// put function definitions here:
#define IR_SENSOR_PIN 6 //QRE 1113
#define IR_VIBROMETER_PIN 13
#define TICKS_PER_REVOLUTION 1  // Anzahl der IR Impulse pro Umdrehung 
#define CLOCK_PIN 47
#define MOSI_PIN 48
#define MISO_PIN 14
#define MUX1_SYNC_PIN 1
#define MUX2_SYNC_PIN 9
#define LED_CLOCK_PIN 12
#define LED_DATA_PIN 11
#define LED_CS_PIN -1
#define colorSaturation 128



//GLOBALS:
const uint16_t pixelCount = 52;
const uint16_t sensorCount = 52;
volatile unsigned long ticksCount = 0;
unsigned long lastRotationCalcTime = 0;
// Variablen für die Berechnungsfrequenz und den Multiplikator für RPM
int rpmCalcFrequency = 20; // in Millisekunden
float rpmMultiplier = 60000.0 / (rpmCalcFrequency * TICKS_PER_REVOLUTION);
// two arrays for the position sensors (left & right arm) 
float magneticArray1[26];
float magneticArray2[26];

// init two instances of the adg731 32ch mux
ADG731 mux1(CLOCK_PIN, MOSI_PIN, MUX1_SYNC_PIN);
ADG731 mux2(CLOCK_PIN, MOSI_PIN, MUX2_SYNC_PIN);
// Define the SPI settings for the sensors
SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE0);
// init the NeoPixelBus instance with Spi and alternate Pins
NeoPixelBus<DotStarBgrFeature, DotStarEsp32DmaSpi3Method> strip(pixelCount);

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

// Callback-Funktion für den Interrupt, wenn der IR-Sensor ausgelöst wird
void RPM_Interrupt() {
  ticksCount++;  // Inkrementiere die Impulsanzahl
}

float* polr2cart (float r, float theta) {
  float cartesian[2];
  float x = r * cos(theta);
  float y = r * sin(theta);
  cartesian[0]=x;
  cartesian[1]=y;
  return cartesian;
}

float* cart2polr (float x, float y){
  float polar[2];
  float r = sqrt( pow(x, 2) + pow(y, 2) );
  float theta = atan(x/y);
  polar[0]=r;
  polar[1]=theta * 180/PI;
  return polar;
}

void setup() {

  Serial.begin(115200);
  SPI.begin(); // Initialize SPI

  // Konfiguriere den IR-Sensor als Eingang und aktiviere den Interrupt
  pinMode(IR_SENSOR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), RPM_Interrupt, RISING);
  pinMode(IR_VIBROMETER_PIN, INPUT);
  // start the strip on the defined SPI bus and init to böack = all pixels off
  strip.Begin(LED_CLOCK_PIN, LED_DATA_PIN, LED_DATA_PIN, LED_CS_PIN);
  strip.ClearTo(black);   // this resets all the DotStars to an off state
  strip.Show();


//###################ESPNow Stuff#############################
 // Initialisiere ESPNow
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESPNow Initialisierung fehlgeschlagen");
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
    Serial.println("Fehler beim Hinzufügen des Peers");
    ESP.restart();
  }
//###########################################################

  float* result = polr2cart(2, 3);
  float x = result[0];
  float y = result[1];
}

void checkRPM(){
  if (millis() - lastRotationCalcTime >= rpmCalcFrequency) {
    lastRotationCalcTime = millis();

    // Berechne die Drehgeschwindigkeit in RPM
    unsigned long rpm = ticksCount * rpmMultiplier;

    // Sende die Drehgeschwindigkeit an den ESP mit Motorshield
    esp_now_send(NULL, (uint8_t*)&rpm, sizeof(unsigned long));

    Serial.print("Drehgeschwindigkeit (RPM): ");
    Serial.println(rpm);

    // Setze die Impulsanzahl zurück
    ticksCount = 0;
  }
}
float readMagneticSensor(){
   // For the sake of this example, return a placeholder value
  float rnd = random(1024);
  return rnd;
}

void readPosition(){
     for (int sensorIndex = 0; sensorIndex < sensorCount; sensorIndex++) {
    // Select the appropriate channel on the first multiplexer
    int mux1Channel = sensorIndex % 26;  // 26 channels on the first multiplexer
    mux1.setChannel(mux1Channel);

    // Activate the first multiplexer as chip select for the sensor
  

    // Read from the SPI-controlled sensor
    float sensorValue1 = readMagneticSensor();

    // Store the sensor value in the array
    magneticArray1[mux1Channel] = sensorValue1;

    // Deactivate the first multiplexer
  

    // Select the appropriate channel on the second multiplexer
    int mux2Channel = sensorIndex % 26;  // 26 channels on the second multiplexer
    mux2.setChannel(mux2Channel);

    // Activate the second multiplexer as chip select for the sensor
  

    // Read from the SPI-controlled sensor
    float sensorValue2 = readMagneticSensor();

    // Store the sensor value in the array
    magneticArray2[mux2Channel] = sensorValue2;

    // Deactivate the second multiplexer


    // // Print the sensor index and values
    // Serial.print("Sensor ");
    // Serial.print(sensorIndex + 1);  // Sensor index starts from 1
    // Serial.print(": ");
    // Serial.print("MUX1 - ");
    // Serial.print(sensorValue1);
    // Serial.print(", MUX2 - ");
    // Serial.println(sensorValue2);

  }
}

void loop() {
  // put your main code here, to run repeatedly:
  checkRPM();
  
    //MUX1.setChannel(ch);
  readPosition();
}

