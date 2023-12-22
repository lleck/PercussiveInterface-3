#define triggerThreshold 10 // If this is set too low, hits on other pads will trigger a "hit" on this pad
#define initialHitReadDuration 500 // In microseconds. Shorter times will mean less latency, but less accuracy. 500 microseconds is nothing, anyway
#define midiVelocityScaleDownAmount 2 // Number of halvings that will be applied to MIDI velocity
#define inputPin A0
 
// Getting the ideal balance of these two constants will ensure that fast subsequent hits are perceived accurately, but false hits are not generated
#define subsequentHitThreshold 1.7
#define subsequentHitThresholdDecaySpeed 14
 
uint16_t highestYet;
uint32_t startReadingTime;
uint32_t highestValueTime;
boolean hitOccurredRecently = false;
 
void setup() {
}
 
void loop() {
 
  // Assume the normal hit-threshold
  uint16_t thresholdNow = triggerThreshold;
 
  // But, if a hit occurred very recently, we need to set a higher threshold for triggering another hit, otherwise the dissipating vibrations
  // of the previous hit would trigger another one now
  if (hitOccurredRecently) {
 
      // Work out how high a reading we'd need to see right now in order to conclude that another hit has occurred
      uint16_t currentDynamicThreshold = (highestYet >> ((micros() - highestValueTime) >> subsequentHitThresholdDecaySpeed)) * subsequentHitThreshold;
 
      // If that calculated threshold is now as low as the regular threshold, we can go back to just waiting for a regular, isolated hit
      if (currentDynamicThreshold <= triggerThreshold) hitOccurredRecently = false;
 
      // Otherwise, do use this higher threshold
      else thresholdNow = currentDynamicThreshold;
  }
 
  // Read the piezo
  uint16_t value = analogRead(inputPin);
 
  // If we've breached the threshold, it means we've got a hit!
  if (value >= thresholdNow) {
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
 
    // Send the MIDI note
    usbMIDI.sendNoteOn(0, (highestYet >> midiVelocityScaleDownAmount) + 1, 1); // We add 1 onto the velocity so that the result is never 0, which would mean the same as a note-off
    Serial.println(highestYet); // Send the unscaled velocity value to the serial monitor too, for debugging / fine-tuning
    hitOccurredRecently = true;
  }
}
 
// Compares times without being prone to problems when the micros() counter overflows, every ~70 mins
boolean timeGreaterOrEqual(uint32_t lhs, uint32_t rhs) {
  return (((lhs - rhs) & 2147483648) == 0);
}