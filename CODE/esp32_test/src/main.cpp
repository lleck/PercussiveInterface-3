
#define DEBUG 1 // 0 to disable serial prints

#if DEBUG
#define D_SerialBegin(...) WebSerial.begin(__VA_ARGS__);
#define D_msgCallback(...) WebSerial.msgCallback(__VA_ARGS__);
#define D_print(...) WebSerial.print(__VA_ARGS__)
#define D_write(...) WebSerial.write(__VA_ARGS__)
#define D_println(...) WebSerial.println(__VA_ARGS__)
#else
#define D_SerialBegin(...)
#define D_msgCallback(...) 
#define D_print(...)
#define D_write(...)
#define D_println(...)
#endif

#include <Arduino.h>
#include <myTMAG5170.h>
#include <FastLED.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h>
#include <WebSerial.h>


#define LED_CLOCK_PIN 12
#define LED_DATA_PIN 11
#define NUM_LEDS 52

#define CLOCK_PIN 47
#define MOSI_PIN 48
#define MISO_PIN 14
#define MUX1_SYNC_PIN 1
#define MUX2_SYNC_PIN 9
#define TMAG_CS_PIN 21
#define IR_VIBROMETER_PIN 13
#define IR_SENSOR_PIN 6 // Rotation Interrupt
#define ROT_TRESH 500

// Globals
const uint8_t pixelCount = 52;
const uint8_t sensorCount = 26;
const uint16_t angularDivisions = 360;

volatile unsigned long ticksCount = 0;
volatile unsigned long timestamp = 0;
volatile unsigned long rotTime, loopTimeNow, loopTimeOld, timeOld, timeNow, divTime;
// currently active sensor (used for debugging)
uint8_t currentSensor = 0;
// Zähler für die aktuelle angulare Division
uint16_t numDiv = 0;
// two 2D array buffers for the magnetic sensors (left & right arm)
int magneticArray1[(sensorCount * 2) + 1][angularDivisions];
int magneticArray2[(sensorCount * 2) + 1][angularDivisions];

// Channel select commands for ADG731 Mux (center first)
uint8_t mux1_ch[27] = {128, 30, 31, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 17, 18, 19, 20, 21, 22, 23};
uint8_t mux2_ch[27] = {128, 23, 22, 21, 20, 19, 18, 17, 16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 31, 30};

CRGB leds[NUM_LEDS];
uint8_t brightness;
uint8_t led_ch[52] = {51, 0, 50, 1, 49, 2, 48, 3, 47, 4, 46, 5, 45, 6, 44, 7, 43, 8, 42, 9, 41, 10, 40, 11, 39, 12, 38, 13, 37, 14, 36, 15, 35, 16, 34,
                      17, 33, 18, 32, 19, 31, 20, 30, 21, 29, 22, 28, 23, 27, 24, 26, 25};

// init the TMAG5170 Class
TMAG5170 magneticSensor;
// init the NeoPixelBus instance with Spi and alternate Pins
// NeoPixelBus<DotStarBgrFeature, DotStarMethod> strip(pixelCount, LED_CLOCK_PIN, LED_DATA_PIN);

// ESP_NOW :: Konfiguriere Peer-Adresse (MAC-Adresse des ESP)
// weitere peers können hier angelegt werden
uint8_t peerAddress[] = {0xBC, 0xFF, 0x4D, 0xF8, 0x84, 0x55};
esp_now_peer_info_t peerInfo;

// Replace with your network credentials
const char* ssid     = "Drum-Access-Point";
const char* password = "supersafe";

// Set web server port number to 80
AsyncWebServer server(80);


// // Structure to send data
// // Must match the receiver structure
// typedef struct struct_message {
//   int divTime;
//   int b;
//   float c;
//   bool d;
// } struct_message;

// // Create a struct_message called myData
// struct_message myData;


SPISettings muxSPI(1000000, MSBFIRST, SPI_MODE2); // spi configuration for the ADG731

// Task related definitions
TaskHandle_t rotCount;
SemaphoreHandle_t rotSem;

// forward declaring functions as this is not written in Arduino IDE
void sensorChannel(uint8_t muxNr, uint8_t channelNr);
void readPosition();
void rotationCounter(void *pvParameters);
int readMagneticSensor();
void recvMsg(uint8_t *data, size_t len);

void setup()
{
  // Connect to Wi-Fi network with SSID and password
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  Serial.begin(115200);
  delay(200);
  D_SerialBegin(&server);
  D_msgCallback(recvMsg);
  server.begin();
  delay(10000);
  Serial.println("WifiSerial Setup!");

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  FastLED.addLeds<APA102, LED_DATA_PIN, LED_CLOCK_PIN, BGR>(leds, NUM_LEDS); // BGR ordering is typical

  SPI.begin(CLOCK_PIN, MISO_PIN, MOSI_PIN);
  pinMode(MUX1_SYNC_PIN, OUTPUT); // set the SS pin as an output
  pinMode(MUX2_SYNC_PIN, OUTPUT); // set the SS pin as an output
  pinMode(IR_SENSOR_PIN, INPUT);

  digitalWrite(MUX1_SYNC_PIN, HIGH);
  digitalWrite(MUX2_SYNC_PIN, HIGH);

  magneticSensor.begin(TMAG_CS_PIN);

  bool error = false;

  // erstelle ein Semaphor für den Task rotationCounter
  rotSem = xSemaphoreCreateMutex();
  // erstelle einen realtime task für die Rotationsabfrage (Core 1 wie der Rest?)
  xTaskCreatePinnedToCore(
      rotationCounter,   // Function to implement the task
      "rotationCounter", // Name of the task
      2048,              // Stack size in bytes erstmal hoch, dann ermitteln mit
                         // uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
      NULL,              // Task input parameter
      1,                 // Priority of the task
      &rotCount,         // Task handle.
      1                  // Core where the task should run
  );

  //  // start the strip on the defined SPI bus and init to black = all pixels off
  // strip.Begin();
  // strip.ClearTo(black); // this resets all the DotStars to an off state
  // strip.Show();
  FastLED.clear(); // clear all pixel data
  brightness = 40;
  FastLED.setBrightness(brightness);
  FastLED.show();

  for (int i = 1; i <= sensorCount && !error; i++)
  {
    sensorChannel(1, i);
    delay(1);
    magneticSensor.disable_crc();
    delay(1);
    magneticSensor.default_cfg(&error);
    delay(1);
    leds[led_ch[i - 1]] = CRGB::GreenYellow;
    FastLED.show();
    delay(50);
    leds[led_ch[i - 1]] = CRGB::Black;
    FastLED.show();
  }
  // turn all channels off on mux 1
  sensorChannel(1, 0);

  for (int j = 1; j <= sensorCount && !error; j++)
  {
    sensorChannel(2, j);
    delay(1);
    magneticSensor.disable_crc();
    delay(1);
    magneticSensor.default_cfg(&error);
    delay(1);
    leds[led_ch[(j + sensorCount) - 1]] = CRGB::CornflowerBlue;
    FastLED.show();
    delay(50);
    leds[led_ch[(j + sensorCount) - 1]] = CRGB::Black;
    FastLED.show();
    //
  }
  // turn all channels off on mux 2
  sensorChannel(2, 0);

  if (error)
  {
    D_print("error conf. sensor nr. "); // Error check
    D_println(currentSensor);
  }


  // uint32_t cpuClock = getCpuFrequencyMhz();
  // D_print("CPUfrequency = ");
  // D_println(cpuClock);
  // D_println("TMAG_config_complete");
  // D_print("MAC-Address: ");
  // D_println(WiFi.macAddress());

  // ###################ESPNow Stuff#############################
  //  Initialisiere ESPNow

  if (esp_now_init() != ESP_OK)
  {
    D_println("ESPNow Initialisierung fehlgeschlagen");
    ESP.restart();
  }

  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Füge den Peer hinzu
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    D_println("Fehler beim Hinzufügen des Peers");
    ESP.restart();
  }
  // ###########################################################
  Serial.println("ESP now setup!");
}

//Webserial Input 
void recvMsg(uint8_t *data, size_t len){
  D_println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  D_println(d);
  // if (d == "ON"){
  //   digitalWrite(LED, HIGH);
  // }
  // if (d=="OFF"){
  //   digitalWrite(LED, LOW);
  // }
}
// Funktion die den IR-Sensor überwacht
// Task // mutex rotSem für Zugriff auf rotTime, divTime, ticksCount
void rotationCounter(void *pvParameters)
{
  pinMode(IR_SENSOR_PIN, INPUT);
  while (true)
  {
    int rotVal = analogRead(IR_SENSOR_PIN);
    //D_println(rotVal);
    if (rotVal < ROT_TRESH)
    {

      xSemaphoreTake(rotSem, portMAX_DELAY); // beanspruche das Semaphor für den Zufriff auf die folgenden Variablen
      timeNow = micros();
      rotTime = timeNow - timeOld;          // substraktion mit unsigned long, daher kein problem mit overflow
      divTime = rotTime / angularDivisions; //
      timeOld = timeNow;
      ticksCount++;           // Inkrementiere die Impulsanzahl
      xSemaphoreGive(rotSem); // stelle das Semaphor für die Dauer eines Tick zur verfügung
      vTaskDelay(1);
    }
  }
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
  for (int sensorIndex = 0; sensorIndex <= 51; sensorIndex++)
  {
    // bei geraden Indexzahlen
    if (sensorIndex % 2 == 0)
    {
      sensorChannel(1, (sensorIndex / 2) + 1);
      // Read from the SPI-controlled sensor
      int sensorValue1 = readMagneticSensor();
      // D_print(sensorValue1);
      // D_print("\t");
      // Store the sensor value in the array
      magneticArray1[sensorIndex][numDiv] = sensorValue1;
      sensorChannel(1, 0);

      // Select the appropriate channel on the second multiplexer
      sensorChannel(2, (sensorIndex / 2) + 1);
      // Read from the SPI-controlled sensor
      int sensorValue2 = readMagneticSensor();
      // D_println(sensorValue2);
      // Store the sensor value in the array
      magneticArray2[sensorIndex][numDiv] = sensorValue2;
      sensorChannel(2, 0);
    }
    else
    { // bei ungeraden Indexzahlen
      if (sensorIndex == 51)
      {
        magneticArray1[sensorIndex][numDiv] = magneticArray1[sensorIndex - 1][numDiv];
        magneticArray2[sensorIndex][numDiv] = magneticArray2[sensorIndex - 1][numDiv];
      }
      else
      {
        magneticArray1[sensorIndex][numDiv] = (magneticArray1[sensorIndex + 1][numDiv] + magneticArray1[sensorIndex - 1][numDiv]) / 2;
        magneticArray2[sensorIndex][numDiv] = (magneticArray2[sensorIndex + 1][numDiv] + magneticArray2[sensorIndex - 1][numDiv]) / 2;
      }
    }
  }
  // numDiv++;
  // if (numDiv >= angularDivisions)
  numDiv = 0;
  D_println("read all positions");
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

  // delay(200);
  //  // u_long start_time = micros();
  readPosition();
  // // u_long end_time = micros();
  // // u_long duration = end_time - start_time;
  // // D_println(duration);
  //  delay(50);

  for (int j = 0; j < sensorCount * 2; j++)
  {
    if (magneticArray1[j][0] > 180)
    {
      uint8_t colorSaturation = map(magneticArray1[j][0], 0, 32768, 0, 120);

      leds[led_ch[j]].setRGB(0, colorSaturation, 0);
    }
    else
    {
      leds[led_ch[j]] = CRGB::Black;
    }

    FastLED.show();
  }
  

  // int rotVal = analogRead(IR_SENSOR_PIN);
  // xSemaphoreTake(rotSem, portMAX_DELAY);
    // char message[] = "Hi, this is a message from the transmitting ESP";
    // esp_now_send(peerAddress, (uint8_t *) message, sizeof(message));
  // xSemaphoreGive(rotSem); // stelle das Semaphor für die Dauer eines Tick zur verfügung
  // vTaskDelay(1);

}
