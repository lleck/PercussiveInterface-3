import controlP5.*;
ControlP5 cp5;

int skinX, skinY;
int radius;
float secondsRadius;
float clockDiameter;
float cps;

Slider s1, s2;

void setup() {
  cp5 = new ControlP5(this);
  size(1280, 720);
  stroke(255);
  
  radius = 255;
  skinX = 2*  width / 3;
  skinY = height / 2;
  
  s1=   cp5.addSlider("cps")
         .setPosition(100,200)
         .setRange(0,20)
         .setValue(10)
         ;
      
  s2=   cp5.addSlider("m_radius")
         .setPosition(100,250)
         .setRange(0,255)
         .setValue(1000)
         .setUpdate(true)
         ;
}

void draw() {
  background(0);
  
  // Draw the skin background
  fill(80);
  ellipse(skinX, skinY, radius*2, radius*2);
  
  // Angles for sin() and cos() start at 3 o'clock;
  // subtract HALF_PI to make them start at the top
  float s = map(millis(), 0, 1/cps, 0, TWO_PI) - HALF_PI;
  float lineX2 = skinX + cos(s) * radius;
  float lineY2 = skinY + sin(s) * radius;
  // Draw the rotor
  stroke(255);
  strokeWeight(10);
  
  line(skinX, skinY, lineX2,lineY2);
  float mouseRadius = dist(mouseX, mouseY, skinX, skinY);
   if( mouseRadius < radius) {
     s2.setValue(mouseRadius);
     println(mouseRadius);
   }
}
