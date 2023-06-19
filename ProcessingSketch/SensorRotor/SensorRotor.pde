import controlP5.*;
ControlP5 cp5;

int skinX, skinY;
int radius;

float impactX;  
float impactY;
float impactR = 10;  //radius of impact

float lineY;
float lineX;

float secondsRadius;
float clockDiameter;
float cps;
float cps_max = 30;

Slider s1, s2, s3;

void setup() {
  cp5 = new ControlP5(this);
  size(1280, 720);
 
  radius = 255;
  skinX = 2*  width / 3;
  skinY = height / 2;
  
  s1=   cp5.addSlider("cps")
         .setPosition(100,200)
         .setRange(0,cps_max)
         .setValue(1)
         ;
      
  s2=   cp5.addSlider("m_radius")
         .setPosition(100,300)
         .setRange(0,255)
         .setValue(1000)
         .setUpdate(true)
         ;
  s3=   cp5.addSlider("rpm")
         .setPosition(100,250)
         .setRange(0,cps_max*60)
         .setValue(cps*60)
         .setUpdate(true)
         ;
}

void draw() {
  background(0);
  
  impactX = mouseX;
  impactY = mouseY;
  
  // Draw the skin background
  fill(80);
  stroke(255);
  ellipse(skinX, skinY, radius*2, radius*2);
  
  // Angles for sin() and cos() start at 3 o'clock;
  // subtract HALF_PI to make them start at the top
  float s = map(millis(), 0, 1000 * 1/cps, 0, TWO_PI) - HALF_PI;
  lineX = skinX + cos(s) * radius;
  lineY = skinY + sin(s) * radius;

  // Draw the rotor

  strokeWeight(10);
   // check for collision
  // if hit, change line's stroke color
  boolean hit = lineCircle(skinX, skinY, lineX, lineY, impactX, impactY, impactR);
  if (hit) stroke(0,50,255, 255);
  else stroke(0,150,255, 150);
  line(skinX, skinY, lineX,lineY);
  
  float mouseRadius = dist(impactX, impactY, skinX, skinY);
   if( mouseRadius < radius) {
     // draw the impact_circle
     noStroke();
     if (hit){
       fill(255,0,0, 255);
       ellipse(impactX,impactY, impactR*2,impactR*2);
       s2.setValue(mouseRadius);
     }
     else 
     {
       fill(100,100,0, 200);
       ellipse(impactX,impactY, impactR,impactR);
     }
   }
   s3.setValue(cps*60);
}

// LINE/CIRCLE
boolean lineCircle(float x1, float y1, float x2, float y2, float cx, float cy, float r) {

  // is either end INSIDE the circle?
  // if so, return true immediately
  boolean inside1 = pointCircle(x1,y1, cx,cy,r);
  boolean inside2 = pointCircle(x2,y2, cx,cy,r);
  if (inside1 || inside2) return true;

  // get length of the line
  float distX = x1 - x2;
  float distY = y1 - y2;
  float len = sqrt( (distX*distX) + (distY*distY) );

  // get dot product of the line and circle
  float dot = ( ((cx-x1)*(x2-x1)) + ((cy-y1)*(y2-y1)) ) / pow(len,2);

  // find the closest point on the line
  float closestX = x1 + (dot * (x2-x1));
  float closestY = y1 + (dot * (y2-y1));

  // is this point actually on the line segment?
  // if so keep going, but if not, return false
  boolean onSegment = linePoint(x1,y1,x2,y2, closestX,closestY);
  if (!onSegment) return false;

  

  // get distance to closest point
  distX = closestX - cx;
  distY = closestY - cy;
  float distance = sqrt( (distX*distX) + (distY*distY) );

  if (distance <= r) {
    return true;
  }
  return false;
}


// POINT/CIRCLE
boolean pointCircle(float px, float py, float cx, float cy, float r) {

  // get distance between the point and circle's center
  // using the Pythagorean Theorem
  float distX = px - cx;
  float distY = py - cy;
  float distance = sqrt( (distX*distX) + (distY*distY) );

  // if the distance is less than the circle's
  // radius the point is inside!
  if (distance <= r) {
    return true;
  }
  return false;
}


// LINE/POINT
boolean linePoint(float x1, float y1, float x2, float y2, float px, float py) {

  // get distance from the point to the two ends of the line
  float d1 = dist(px,py, x1,y1);
  float d2 = dist(px,py, x2,y2);

  // get the length of the line
  float lineLen = dist(x1,y1, x2,y2);

  // since floats are so minutely accurate, add
  // a little buffer zone that will give collision
  float buffer = 0.1;    // higher # = less accurate

  // if the two distances are equal to the line's
  // length, the point is on the line!
  // note we use the buffer here to give a range,
  // rather than one #
  if (d1+d2 >= lineLen-buffer && d1+d2 <= lineLen+buffer) {
    return true;
  }
  return false;
}
