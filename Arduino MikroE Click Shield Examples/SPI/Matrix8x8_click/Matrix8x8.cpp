/*
 * 8x8 (R/G/Y/B) Click 
 * Version 1.0 May 2014.
 * Copyright 2014 Dejan Odabasic - mikroElektronika
 */
#include "Arduino.h"
#include "Matrix8x8.h"

Matrix8x8::Matrix8x8(byte chipSelect){
  csPin = chipSelect;
}

void Matrix8x8::init() 
{
  pinMode (csPin, OUTPUT);
  setCommand(max7219_scanLimit,   0x07);  // For matrix8x8 all 8 display should be driven     
  setCommand(max7219_decodeMode,  0x00);  // using an led matrix (not digits)
  setCommand(max7219_shutdown,    0x01);  // not in shutdown mode
  setCommand(max7219_displayTest, 0x00);  // no display test
  setCommand(max7219_intensity,   0x01);  // Set minimal intensity
}

void Matrix8x8::setCommand(byte cmd, byte data)
{
  digitalWrite(csPin, LOW);
//  for (int i=0; i<num; i++) 
//  {
    SPI.transfer(cmd);
    SPI.transfer(data);
//  }
  digitalWrite(csPin, HIGH);
}

void Matrix8x8::setIntensity(byte intensity)
{
  // Valid valus are 0-15 (0x0-0xF)
  if(intensity <= 0xF)
    setCommand(max7219_intensity, intensity);
}

void Matrix8x8::clear()
{
  for (int i = 0; i < 8; i++) 
    setColumn(i,0);
}

void Matrix8x8::setColumn(byte col, byte value){
  // Register for Digit 0 is located at 0x01 
  //  that's why  (col) need to be incremented.
  setCommand(col+1, value);
}
