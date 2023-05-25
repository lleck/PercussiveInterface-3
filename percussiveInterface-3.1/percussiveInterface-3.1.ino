/* Analog Input Example, Teensyduino Tutorial #4
   http://www.pjrc.com/teensy/tutorial4.html

   After uploading this to your board, use Serial Monitor
   to view the message.  When Serial is selected from the
   Tools > USB Type menu, the correct serial port must be
   selected from the Tools > Serial Port AFTER Teensy is
   running this code.  Teensy only becomes a serial device
   while this code is running!  For non-Serial types,
   the Serial port is emulated, so no port needs to be
   selected.

   This example code is in the public domain.
*/

void setup()
{                
  Serial.begin(38400);
    pinMode(A14, INPUT);
    pinMode(A15, INPUT);
    pinMode(A16, INPUT);
}
float val1;
float val2;
float val3;

void loop()                     
{
  val1 = analogRead(A14);
  val2 = analogRead(A15);
  val3 = analogRead(A16);

  Serial.print("val1\t");
  Serial.print(val1);
  Serial.print("    ");
  Serial.print("val2\t");
  Serial.print(val2);
  Serial.print("    ");
  Serial.print("val3\t");
  Serial.println(val3);
  
  delay(5);
}
