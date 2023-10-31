/*
 * For use with 8x8 (R/G/Y/B) Click 
 * Version 1.0 May 2014.
 * Copyright 2014 Dejan Odabasic - mikroElektronika
 */

#ifndef _Matrix8x8_
#define _Matrix8x8_
#include "Arduino.h"
#include <SPI.h>
#define max7219_noop        0x00
#define max7219_digit0      0x01
#define max7219_digit1      0x02
#define max7219_digit2      0x03
#define max7219_digit3      0x04
#define max7219_digit4      0x05
#define max7219_digit5      0x06
#define max7219_digit6      0x07
#define max7219_digit7      0x08
#define max7219_decodeMode  0x09
#define max7219_intensity   0x0a
#define max7219_scanLimit   0x0b
#define max7219_shutdown    0x0c
#define max7219_displayTest 0x0f

class Matrix8x8
{
  private:
    byte csPin;
  public:
    Matrix8x8(byte chipSelect);
   
    void init();
    void clear();
    void setCommand(byte cmd, byte data);
    void setIntensity(byte intensity);
    void setColumn(byte col, byte value);
};

#endif
