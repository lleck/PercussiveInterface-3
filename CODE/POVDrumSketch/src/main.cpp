// ########################################################
//  test code to drive the POV Drum Interface
////////////////////////////////////////////
// BUILDING BLOCKS:
// ########################################################
//*Neopixelbus to drive the APA102 LEDs                   x
//*function to monitor RPM (interrupt routine)            x
//*functions to convert polar2cartesian and backwards     x
//*ESPNow protocol to send out the data                   x
//*library to operate the ADG731 MUXes                    x
//*functions to read out the TMAG5170 hall effect sensors x
//*function to read out impulse sensor (task_TriggerLoop) x
//*function to show test image
// #######################################################

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
#include <math.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h> // to store some initial settings/ tresholds / calibration data
#include <NeoPixelBus.h>
#include <myTMAG5170.h>
#include <FastTrig.h>

// put function definitions here:
#define IR_SENSOR_PIN 6 // QRE 1113
#define IR_VIBROMETER_PIN 13
#define TICKS_PER_REVOLUTION 1 // Anzahl der IR Impulse pro Umdrehung
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
#define RO_MODE true
#define RW_MODE false
#define triggerThreshold 3            // If this is set too low, hits on other pads will trigger a "hit" on this pad
#define initialHitReadDuration 500    // In microseconds. Shorter times will mean less latency, but less velocity-accuracy
#define midiVelocityScaleDownAmount 2 // Number of halvings that will be applied to MIDI velocity

Preferences calibration;
const bool calibrating = false;

uint16_t highestYet;
uint32_t startReadingTime;
uint32_t highestValueTime;
boolean hitOccurredRecently = false;
boolean newRecordSet;
// array to
uint16_t tresholdsFactor[256];

// GLOBALS:
const uint8_t pixelCount = 52;
const uint8_t sensorCount = 26;
const uint16_t angularDivisions = 360;

volatile unsigned long ticksCount = 0;
volatile unsigned long timestamp = 0;
volatile unsigned long rotTime, loopTimeNow, loopTimeOld, timeOld, timeNow, divTime;
unsigned long lastRotationCalcTime = 0;
// Zähler für die aktuelle angulare Division
uint16_t numDiv = 0;
// Variablen für die Berechnungsfrequenz und den Multiplikator für RPM
unsigned long rpm;
uint8_t rpmCalcFrequency = 100; // in Millisekunden
float rpmMultiplier = 60000.0 / (rpmCalcFrequency * TICKS_PER_REVOLUTION);
// two 2D array buffers for the magnetic sensors (left & right arm)
int magneticArray1[sensorCount][angularDivisions];
int magneticArray2[sensorCount][angularDivisions];
// currently active sensor (used for debugging)
uint8_t currentSensor = 0;

// Channel select commands for ADG731 Mux (center first)
uint8_t mux1_ch[26] = {30, 31, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 17, 18, 19, 20, 21, 22, 23};
uint8_t mux2_ch[26] = {23, 22, 21, 20, 19, 18, 17, 16, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 12, 13, 14, 15, 31, 30};
uint8_t led_ch[52] = {51, 0, 50, 1, 49, 2, 48, 3, 47, 4, 46, 5, 45, 6, 44, 7, 43, 8, 42, 9, 41, 10, 40, 11, 39, 12, 38, 13, 37, 14, 36, 15, 35, 16, 34,
                      17, 33, 18, 32, 19, 31, 20, 30, 21, 29, 22, 28, 23, 27, 24, 26, 25};

SPISettings muxSPI(10000000, MSBFIRST, SPI_MODE2); // spi configuration for the ADG731

// init the TMAG5170 Class
TMAG5170 magneticSensor;
// init the NeoPixelBus instance with Spi and alternate Pins
NeoPixelBus<DotStarBgrFeature, DotStarEsp32DmaSpi3Method> strip(pixelCount);

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

// forward declaring functions as this is not written in Arduino IDE
float *polr2cart(float r, float theta);
float *cart2polr(float x, float y);
void IRAM_ATTR Rotation_Interrupt();
void triggerLoop(void *pvParameters);
void sensorChannel(uint8_t muxNr, uint8_t channelNr);
void mux_off(int muxNr);
void readPosition();
int readMagneticSensor();

void setup()
{
  D_SerialBegin(115200);
  SPI.begin(CLOCK_PIN, MISO_PIN, MOSI_PIN);
  pinMode(MUX1_SYNC_PIN, OUTPUT); // set the CS pin as an output
  pinMode(MUX2_SYNC_PIN, OUTPUT); // set the CS pin as an output
  pinMode(TMAG_CS_PIN, OUTPUT);   // set the CS pin as an output
  digitalWrite(MUX1_SYNC_PIN, HIGH);
  digitalWrite(MUX2_SYNC_PIN, HIGH);
  digitalWrite(TMAG_CS_PIN, HIGH);

  bool error = false;

  magneticSensor.begin(TMAG_CS_PIN);
  magneticSensor.disable_crc();

  // erstelle einen realtime task für die Triggerabfrage (Core 1 wie der Rest?)
  // Welche Variablen werden außerhalb genutzt ? semaphor benötigt ?
  xTaskCreatePinnedToCore(
      triggerLoop,   // Function to implement the task
      "triggerLoop", // Name of the task
      8192,          // Stack size in bytes erstmal hoch, dann ermitteln mit
                     // uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
      NULL,          // Task input parameter
      1,             // Priority of the task
      NULL,          // Task handle.
      1              // Core where the task should run
  );

  // Konfiguriere den IR-Sensor als Eingang und aktiviere den Interrupt
  pinMode(IR_SENSOR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), Rotation_Interrupt, FALLING);
  pinMode(IR_VIBROMETER_PIN, INPUT);

  // start the strip on the defined SPI bus and init to black = all pixels off
  strip.Begin(LED_CLOCK_PIN, LED_DATA_PIN, LED_DATA_PIN, LED_CS_PIN);
  strip.ClearTo(black); // this resets all the DotStars to an off state
  strip.Show();

  // initialisieren des TAG5170 arrays / set to each

  for (int i = 0; i < sensorCount && !error; i++)
  {
    sensorChannel(1, i);
    magneticSensor.default_cfg(&error);
  }
  mux_off(1);
  for (int j = 0; j < sensorCount && !error; j++)
  {
    sensorChannel(2, j);
    magneticSensor.default_cfg(&error);
  }
  mux_off(2);
  if (error)
  {
    D_print("error conf. sensor nr. "); // Error check
    D_println(currentSensor);
  }

  // ###############check for calibration_Mode##################
  if (calibrating)
  {
    calibration.begin("th", RW_MODE); // namespace th verwenden (anlegen) + schreibrechte
    calibration.clear();              // clear previous keys in the namespace
    for (int i = 0; i < 256; i++)
    { // array values zurücksetzen
      tresholdsFactor[i] = 0;
    }
    // speichern des arrays als reihe von bytes
    calibration.putBytes("th", &tresholdsFactor, sizeof(tresholdsFactor));
  }
  else
  {
    calibration.begin("th", RO_MODE); // keine schreibrechte
    // put the bytes from flash into the array
    calibration.getBytes("th", &tresholdsFactor, sizeof(tresholdsFactor));
  }
  // ###################ESPNow Stuff#############################
  //  Initialisiere ESPNow
  if (esp_now_init() != ESP_OK)
  {
    D_println("ESPNow Initialisierung fehlgeschlagen");
    ESP.restart();
  }
  // Konfiguriere Peer-Adresse (MAC-Adresse des ESP)
  // weitere peers können hier angelegt werden
  uint8_t peerAddress[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
  esp_now_peer_info_t peerInfo;
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

  // float *cartResult = polr2cart(2, 3);
  // float x = cartResult[0];
  // float y = cartResult[1];

  // float *polrResult = cart2polr(-5, 5);
  // float r = polrResult[0];
  // float theta = polrResult[1];
}

float *polr2cart(float r, float theta)
{
  float cartesian[2];
  // verwendung von FastTrig überprüfen !!!!
  float x = r * icos(theta);
  float y = r * isin(theta);
  cartesian[0] = x;
  cartesian[1] = y;
  return cartesian;
}

float *cart2polr(float x, float y)
{
  float polar[2];
  float r = hypotFast(x, y);     // präzision muss überprüft werden !!!
  float theta = atan2Fast(x, y); // darf nicht 0/0 sein
  polar[0] = r;
  polar[1] = theta * 180 / PI;
  return polar;
}

// Callback-Funktion für den Interrupt, wenn der IR-Sensor ausgelöst wird
// kann nur als Interrupt angelegt werden, wenn Noise klein genug, sonst Task ? // mutex rotTime divTime
void IRAM_ATTR Rotation_Interrupt()
{
  timeNow = micros();
  rotTime = timeNow - timeOld;          // substraktion mit unsigned long, daher kein problem mit overflow
  divTime = rotTime / angularDivisions; //
  timeOld = timeNow;
  ticksCount++; // Inkrementiere die Impulsanzahl
}

void checkRPM()
{
  if (millis() - lastRotationCalcTime >= rpmCalcFrequency)
  {
    lastRotationCalcTime = millis();

    // Berechne die Drehgeschwindigkeit in RPM
    unsigned long rpm = ticksCount * rpmMultiplier;

    // Sende die Drehgeschwindigkeit an den ESP mit Motorshield
    // esp_now_send(NULL, (uint8_t*)&rpm, sizeof(unsigned long));

    D_print("Drehgeschwindigkeit (RPM): ");
    D_println(rpm);

    // Setze die Impulsanzahl zurück
    ticksCount = 0;
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

void readPosition()
{
  for (int sensorIndex = 0; sensorIndex < sensorCount * 2; sensorIndex++)
  {
    // bei geraden Indexzahlen
    if (sensorIndex % 2 == 0)
    {
      sensorChannel(1, sensorIndex / 2);
      // Read from the SPI-controlled sensor
      int sensorValue1 = readMagneticSensor();
      // Store the sensor value in the array
      magneticArray1[sensorIndex][numDiv] = sensorValue1;
      mux_off(1);

      // Select the appropriate channel on the second multiplexer
      sensorChannel(2, sensorIndex / 2);
      // Read from the SPI-controlled sensor
      int sensorValue2 = readMagneticSensor();
      // Store the sensor value in the array
      magneticArray2[sensorIndex][numDiv] = sensorValue2;
      mux_off(2);
    }
    // bei ungeraden Indexzahlen
    else
    {
      magneticArray1[sensorIndex][numDiv] = (magneticArray1[sensorIndex + 1][numDiv] + magneticArray1[sensorIndex - 1][numDiv]) / 2;
      magneticArray2[sensorIndex][numDiv] = (magneticArray2[sensorIndex + 1][numDiv] + magneticArray2[sensorIndex - 1][numDiv]) / 2;
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
  numDiv++;
  if (numDiv >= angularDivisions)
  numDiv = 0;
}

// Compares times without being prone to problems when the micros() counter overflows, every ~70 mins
boolean timeGreaterOrEqual(uint32_t lhs, uint32_t rhs)
{
  return (((lhs - rhs) & 2147483648) == 0);
}

void triggerLoop(void *pvParameters)
{
  while (1)
  {
    // Triggerabfrage
    // Assume the normal hit-threshold
    uint16_t thresholdNow = triggerThreshold;
    // Read the Vibrometer
    uint16_t value = analogRead(IR_VIBROMETER_PIN);
    uint32_t msPassed;
    // But, if a hit occurred very recently, we need to set a higher threshold for triggering another hit, otherwise the dissipating vibrations
    // of the previous hit would trigger another one now
    if (hitOccurredRecently)
    {
      uint32_t usPassed = micros() - highestValueTime;
      msPassed = usPassed >> 10;
      if (calibrating)
      {
        //  diser block kommt also nach else
        if (msPassed >= 256)
        {
          hitOccurredRecently = false;
          if (newRecordSet)
          {
            // hau das array zurück in den Flash Speicher
            calibration.putBytes("th", &tresholdsFactor, sizeof(tresholdsFactor));
            // hau die aufgenommenen Zahlen in den Monitor
            D_println("----------------------------------------");
            for (int i = 0; i < 256; i++)
            {
              D_print(tresholdsFactor[i] + ", ");
              if (i % 16 == 0)
                D_println();
            }
          }
        }
        // diser block wird in schleife ausgeführt bis alle 256 positionen im array voll geschrieben sind ?
        else
        {
          // verhältnis neuer Wert zu altem Maximum * 256  //  ceil ( aufrunden )
          uint16_t nowComparedToHighest = ceil((float)value / (float)highestYet * 256);
          uint8_t previousRecord = tresholdsFactor[msPassed];
          if (nowComparedToHighest > previousRecord)
          {
            newRecordSet = true;
            // min (vergleiche und nehme den geringeren wert)
            //  "min" expects both values to be the same type
            tresholdsFactor[msPassed] = std::min<uint16_t>(255, nowComparedToHighest);
          }
        }
        thresholdNow = 1024; // blockiert TRIGGER_BLOCK nach hitOccuredRecently
                             // in diesem Modus wollen wir das Abschwingen aufzeichnen keine Drumrolls spielen
      }
      //---------> not in calibrating mode
      else
      {
        // wenn wir außerhalb der Nachschwingzeit sind, weiter....
        if (msPassed >= 256)
          hitOccurredRecently = false;
        // befinden wir uns innerhalb der Nachschwingzeit, dann wende dynamischen Treshold an....
        else
        {
          // Work out how high a reading we'd need to see right now in order to conclude that another hit has occurred
          uint32_t currentDynamicThreshold;
          // wenn wir uns innerhalb der initialHitReadDuration befinden muss der nächste Schlag um den Betrag
          // von triggerTreshold stärker  sein als der erste Schlag
          if (usPassed < initialHitReadDuration)
            currentDynamicThreshold = highestYet;
          // andernfalls skaliere den Wert mit dem hinterlegten Faktor für diese millisekunde
          else
            currentDynamicThreshold = ((uint32_t)tresholdsFactor[msPassed] * highestYet) >> 8; // >>8 ist geteilt durch 256

          thresholdNow += currentDynamicThreshold;
        }
      }
    }
    // TRIGGER-BLOCK
    // If we've breached the threshold, it means we've got a hit!
    if (value >= thresholdNow)
    {
      startReadingTime = micros();
      highestYet = 0;
      // For the next few milliseconds, look out for the highest "spike" in the reading from the piezo. Its height is representative of the hit's velocity
      do
      {
        if (value > highestYet)
        {
          highestYet = value;
          highestValueTime = micros();
        }
        value = analogRead(IR_VIBROMETER_PIN);
      } while (timeGreaterOrEqual(startReadingTime + initialHitReadDuration, micros()));
      // Send the data with corresponding position data

      // usbMIDI.sendNoteOn(0, (highestYet >> midiVelocityScaleDownAmount) + 1, 1); // We add 1 onto the velocity so that the result is never 0, which would mean the same as a note-off
      D_println(highestYet); // Send the unscaled velocity value to the serial monitor too, for debugging / fine-tuning
      hitOccurredRecently = true;
      newRecordSet = false;
    }
  }
}

void loop()
{
  xSemaphoreTake(rotSem,portMAX_DELAY);
  loopTimeNow = micros();
  // was kann vielleicht in einen extra Task ?
  // Zeitintervall für die winkelgenaue Positionsabfrage ohne Überlauf dank Subtraktion
  if ((unsigned long)(loopTimeNow - loopTimeOld) >= divTime)
  {
    readPosition();
    // Use the snapshot to set track time until next event
    loopTimeOld = loopTimeNow;
  }
  xSemaphoreGive(rotSem);
  vTaskDelay(1);
  checkRPM();
}
