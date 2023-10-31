/*
 * Project name:
     UART Demonstration
 * Copyright:
     (c) Mikroelektronika, 2014.
 * Revision History:
     20140624:
       - initial release (DO);   
 * Description:
     This project features a simple UART communication example. At the beginning,
     a starting message will appear in the UART terminal. After that, user will
     write a message to the Arduino Uno board, which will be echoed back to the UART Terminal
 * Test configuration:
     MCU:             ATmega328
                      http://www.atmel.com/Images/doc8161.pdf
     Dev. Board:      Arduino Uno
                      http://arduino.cc/en/Main/arduinoBoardUno
     Oscillator:      16.0000 MHz Crystal
     Ext. Modules:    Arduino Uno Click Shield
                      http://www.mikroe.com/click/arduino-uno-shield
     SW:              Arduino IDE
                      http://arduino.cc/en/main/software
 * NOTES:
     - Set the on-board to PROG position when uploading project.
     - After the uploading is completed, set the switch to UART position, 
       connect to the appropriate COM port with the 9600 baud rate.
 */

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

void setup() {
  // initialize serial:
  Serial.begin(9600);
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);
  Serial.println("UART Echo Test"); 
}

void loop() {
  // print the string when a newline arrives:
  if (stringComplete) {
    Serial.println(inputString); 
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    } 
  }
}


