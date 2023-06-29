#include <ADC.h>
#include <ADC_util.h>
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

ADC *adc = new ADC(); // adc object;

void setup()
{                
  Serial.begin(38400);
    pinMode(A10, INPUT);
    pinMode(A11, INPUT); //ref 1.65
    
    pinMode(A12, INPUT);
    pinMode(A13, INPUT); //ref 1.65
    
    adc->adc0->setAveraging(64); // set number of averages
    adc->adc0->setResolution(16); // set bits of resolution
    adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
    adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED); // change the sampling speed

    adc->adc1->setAveraging(64); // set number of averages
    adc->adc1->setResolution(16); // set bits of resolution
    adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
    adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED); // change the sampling speed
   
}
float val1;
float val2;


void loop()                     
{

 
 
  val1 = adc->adc0->analogReadDifferential(A10, A11); 
  val2 = adc->adc1->analogReadDifferential(A12, A13); 


  Serial.print("val1\t");
  Serial.print(val1);
  Serial.print("    ");
  Serial.print("val2\t");
  Serial.println(val2);

  
  delay(5);
}
