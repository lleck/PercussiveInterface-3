#include <Wire.h>

/*
 * Project name:
     EEPROM Click Board Demonstration
 * Copyright:
     (c) Mikroelektronika, 2014.
 * Revision History:
     20140624:
       - initial release (DO);   
 * Description:
     This example features the advanced communication with the 24AA01 EEPROM chip
     by introducing its own library of functions for this task: init, write, and read. 
     It performs a byte write into the EEPROM and then reads the written byte.
     Then, data read from EEPROM is displayed in the UART terminal.
 * Test configuration:
     MCU:             ATmega328
                      http://www.atmel.com/Images/doc8161.pdf
     Dev. Board:      Arduino Uno
                      http://arduino.cc/en/Main/arduinoBoardUno
     Oscillator:      16.0000 MHz Crystal
     Ext. Modules:    EEPROM Click board
                      http://www.mikroe.com/click/eeprom/
                      Arduino Uno Click Shield
                      http://www.mikroe.com/click/arduino-uno-shield
     SW:              Arduino IDE
                      http://arduino.cc/en/main/software
 * NOTES:
     - Place EEPROM click into mikroBUS socket1 of the Arduino Uno Click Shield.
     - Set the on-board to PROG position when uploading project.
     - After the uploading is completed, set the switch to UART position, 
       connect to the appropriate COM port with the 9600 baud rate.
 */

// variables declaration
char c;
byte address;

// setup I2C and UART
void setup()
{
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
  Serial.println("To operate EEPROM, use following commands :");
  Serial.println("1. Enter 'A' to set address, followed by the EEPROM address (0-255), i.e. A0,");
  Serial.println("2. Enter 'W' for writing, followed by the value to be written, i.e. W5,");  
  Serial.println("3. Enter 'R' to read the written value from the set address.");
}

// main loop
void loop()
{
  if(Serial.available()){
    switch((char)Serial.read()){
      // Address mode
      case 'A':  
        address = getCmd();
        Serial.print("Address SET: ");
        Serial.println(address, DEC);
      break;

      // Write mode
      case 'W':
          c = getCmd();
          Wire.beginTransmission(84);
          Wire.write(address);
          Wire.write(c);
          Wire.endTransmission();
          Serial.print("Written to EEPROM: ");
          Serial.println(c);
      break;
      
      // Write mode
      case 'R':
        Wire.beginTransmission(84);
        Wire.write(address);
        Wire.endTransmission();
        Wire.requestFrom(84, 1, true);    // request 6 bytes from slave device #2
        if (Wire.available() > 0){
          while(Wire.available())         // slave may send less than requested
          { 
             c = Wire.read();             // receive a byte as character
            Serial.print(c);              // print the character
          } 
          Serial.println();
        }
        else{
          Serial.println("No data");
        }
      break;
      default:
        Serial.println("Unknown command");
    }
  } 
}

byte getCmd(){
  while(!Serial.available()){
  };
return (char)Serial.read();
}
