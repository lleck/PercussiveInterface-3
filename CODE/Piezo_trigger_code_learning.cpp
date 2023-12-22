
#include <Preferences.h>

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
 
#define RO_MODE true
#define RW_MODE false
#define triggerThreshold 3 // If this is set too low, hits on other pads will trigger a "hit" on this pad
#define initialHitReadDuration 500 // In microseconds. Shorter times will mean less latency, but less velocity-accuracy
#define midiVelocityScaleDownAmount 2 // Number of halvings that will be applied to MIDI velocity
#define inputPin A0

bool calibrating = false; 
Preferences calibration;                        

uint16_t highestYet;
uint32_t startReadingTime;
uint32_t highestValueTime;
boolean hitOccurredRecently = false;
boolean newRecordSet;
uint16_t tresholdsFactor [256];

void setup() {

    if (calibrating) {
        calibration.begin("th", RW_MODE);                     // namespace th verwenden (anlegen) + schreibrechte
        calibration.clear();                                // clear previous keys in the namespace
        for (int i = 0; i < 256; i++) {                     // array values zurücksetzen
            tresholdsFactor[i] = 0;
        }
        calibration.putBytes( "th", &tresholdsFactor, sizeof(tresholdsFactor) );  // speichern des arrays als reihe von bytes
    }
    else {
        calibration.begin("th", RO_MODE);                       // keine schreibrechte 
        calibration.getBytes( "th", &tresholdsFactor, sizeof(tresholdsFactor)); // put the bytes from flash into the buffer 
    }
}
 
void loop() {
 
  // Assume the normal hit-threshold
  uint16_t thresholdNow = triggerThreshold;
 
  // Read the piezo
  uint16_t value = analogRead(inputPin);
 
  uint32_t msPassed;
 
  // But, if a hit occurred very recently, we need to set a higher threshold for triggering another hit, otherwise the dissipating vibrations
  // of the previous hit would trigger another one now
  if (hitOccurredRecently) 
  {
 
      uint32_t usPassed = micros() - highestValueTime;
      msPassed = usPassed >> 10;
 
      if (calibrating) {
        
        //  diser block kommt also nach else 
        if (msPassed >= 256) {
          hitOccurredRecently = false;
          if (newRecordSet) {
            // hau das array zurück in den Flash Speicher
            calibration.putBytes("th", &tresholdsFactor, sizeof(tresholdsFactor));
            // hau die aufgenommenen Zahlen in den Monitor 
            D_println("----------------------------------------");
            for (int i = 0; i < 256; i++) {
              D_print(tresholdsFactor[i] + ", ");
              if (i % 16 == 0) D_println();
            }
          }
        }
        // diser block wird in schleife ausgeführt bis alle 256 positionen im array voll geschrieben sind ? 
        else {
            // verhältnis neuer Wert zu altem Maximum * 256  //  ceil ( aufrunden )
          uint16_t nowComparedToHighest = ceil((float)value / (float)highestYet * 256);
          uint8_t previousRecord = tresholdsFactor[msPassed];
          if (nowComparedToHighest > previousRecord) {
            newRecordSet = true;
            // min (vergleiche und nehme den geringeren wert)
            tresholdsFactor[msPassed] = min(255, nowComparedToHighest);
          }
        }
        thresholdNow = 1024; // blockiert TRIGGER_BLOCK nach hitOccuredRecently 
                             // in diesem Modus wollen wir das Abschwingen aufzeichnen keine Drumrolls spielen  
      }
  //---------> not in calibrating mode    
      else {
        // wenn wir außerhalb der Nachschwingzeit sind, weiter....
        if (msPassed >= 256) hitOccurredRecently = false;
        // befinden wir uns innerhalb der Nachschwingzeit, dann wende dynamischen Treshold an....
        else {
          // Work out how high a reading we'd need to see right now in order to conclude that another hit has occurred
          uint32_t currentDynamicThreshold;
          // wenn wir uns innerhalb der initialHitReadDuration befinden muss der nächste Schlag um den Betrag 
          // von triggerTreshold stärker  sein als der erste Schlag 
          if (usPassed < initialHitReadDuration) currentDynamicThreshold = highestYet;
          //andernfalls skaliere den Wert mit dem hinterlegten Faktor für diese millisekunde 
          else currentDynamicThreshold = ((uint32_t)tresholdsFactor[msPassed] * highestYet) >> 8; // >>8 ist geteilt durch 256
 
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
    do {
      if (value > highestYet) {
        highestYet = value;
        highestValueTime = micros();
      }
      value = analogRead(inputPin);
    } while (timeGreaterOrEqual(startReadingTime + initialHitReadDuration, micros()));
 
    // Send the data with corresponding position data
    
    //usbMIDI.sendNoteOn(0, (highestYet >> midiVelocityScaleDownAmount) + 1, 1); // We add 1 onto the velocity so that the result is never 0, which would mean the same as a note-off
    D_println(highestYet); // Send the unscaled velocity value to the serial monitor too, for debugging / fine-tuning
    hitOccurredRecently = true;
    newRecordSet = false;
  }
}
 
// Compares times without being prone to problems when the micros() counter overflows, every ~70 mins
boolean timeGreaterOrEqual(uint32_t lhs, uint32_t rhs) {
  return (((lhs - rhs) & 2147483648) == 0);
}