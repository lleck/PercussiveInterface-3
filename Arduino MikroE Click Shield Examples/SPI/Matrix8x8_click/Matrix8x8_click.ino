/*
 * Project name:
     8x8 Click Board Demonstration
 * Copyright:
     (c) Mikroelektronika, 2014.
 * Revision History:
     20140624:
       - initial release (DO);   
 * Description:
     This project features a simple demonstration of the 8x8 Click Board - LED matrix.
     The LED matrix will display (simulate) a wave using the LEDs.
 * Test configuration:
     MCU:             ATmega328
                      http://www.atmel.com/Images/doc8161.pdf
     Dev. Board:      Arduino Uno
                      http://arduino.cc/en/Main/arduinoBoardUno
     Oscillator:      16.0000 MHz Crystal
     Ext. Modules:    8x8 Click board
                      http://www.mikroe.com/click/8x8-g/
                      Arduino Uno Click Shield
                      http://www.mikroe.com/click/arduino-uno-shield
     SW:              Arduino IDE
                      http://arduino.cc/en/main/software
 * NOTES:
     - Place 8x8 clicks into mikroBUS socket 1 and 2 of the Arduino Uno Click Shield.
     - Set the on-board to PROG position when uploading project.
     - Notice the wave-like motion on the LED matrix.
 */
 
#include <SPI.h>
#include "Matrix8x8.h"

Matrix8x8 Click1 (10);  // Chip select is D10
Matrix8x8 Click2 (9);   // Chip select is D9
byte matrix[] ={0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};

void setup() {
  // initialize SPI:
  SPI.begin(); 
  
  Click1.init();
  Click2.init();
  /* Don't look directly at LEDs */
  Click1.setIntensity(0xF);   // Watch you EYES!!!
  Click2.setIntensity(0xF);   // Watch you EYES!!!
  for (int i = 0; i < 8; i++){
    Click1.setColumn(i, 0xFF);
    Click2.setColumn(i, 0xFF);
  }
  delay(1500);
  Click1.setIntensity(0x2);
  Click2.setIntensity(0x2);
}

void loop() {
  for (int i = 0; i < 8; i++){
    Click2.setColumn(i, matrix[i]);
    delay(100);
  }
  for (int i = 0; i < 8; i++){
    Click1.setColumn(i, matrix[i]);
    delay(100);
  }
  for(int i = 0; i < 8; i++)
    matrix[i] = ~matrix[i];
  delay(100);
}

