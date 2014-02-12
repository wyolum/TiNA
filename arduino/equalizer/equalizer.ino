#include "Adafruit_NeoPixel.h"

const uint8_t pins[8] = {9, 8, 7, 6, 5, 4, 3, 2};
const uint8_t N_STRIP = 8;

Adafruit_NeoPixel strips[8];

// eq stuff
const int strobe = A0; // strobe pins on digital 4
const int res = A1; // reset pins on digital 5


int eqLevels[7]; // store band values in these arrays


void setup(){
  Serial.begin(115200);
   pinMode(res, OUTPUT); // reset
  pinMode(strobe, OUTPUT); // strobe
  

  for(uint8_t i = 0; i < N_STRIP; i++){
  // all strips share the same buffer
    strips[i].setup(16, pins[i], NEO_GRB + NEO_KHZ800);
    strips[i].begin();
    strips[i].show();
    pinMode(pins[i],OUTPUT);
    digitalWrite(pins[i],LOW);
  }
  clearAll();
  InitMSGEq7();
}

int pixel =0;
int row =0;

void loop(){
  clearAll();
  InitMSGEq7();
  readMSGEQ7();
  for (int i=0; i < 7; i++){
      int level = map(eqLevels[i],0,600,0,16);
      setBar(i,level);
    }
    showall();
    delay(10);
}

void showall(){
  for (int i = 0; i < N_STRIP; i++){
    strips[i].changed = true;
    strips[i].show();
  }
}
// Fill the dots one after the other with a color
void colorFill(uint8_t strip, uint32_t c) {
  for(uint16_t i=0; i<strips[strip].numPixels(); i++) {
      strips[strip].setPixelColor(i, c);
  }
}
void clearAll(){
   for(int row =0;row < N_STRIP; row++){
       for(uint16_t i=0; i<strips[row].numPixels(); i++) {
          strips[row].setPixelColor(i, 0);
       }
   }
}
void setBar(uint8_t row, uint8_t level)
{
   uint32_t color;

  for(uint16_t i=0; i < level; i++) {
     if (i <= 4 )
       color = Color(0,127,0); // green
     else if (i > 4 && i < 10)
       color = Color(80,80,0);
     else
       color = Color(127,0,0);
     strips[row].setPixelColor(i, color);
  }
}
uint32_t Color(uint8_t red, uint8_t green, uint8_t blue){
  return strips[0].Color(red,green,blue);
}
// Example 48.1 - tronixstuff.com/tutorials > chapter 48 - 30 Jan 2013 
// MSGEQ7 spectrum analyser shield - basic demonstration

int band;
int EQ_OUT1 = A2;
void InitMSGEq7()
{
 digitalWrite(res,LOW); // reset low
 digitalWrite(strobe,HIGH); //pin 5 is RESET on the shield
}
void readMSGEQ7()
// Function to read 7 band equalizers
{
 digitalWrite(res, HIGH);
 digitalWrite(res, LOW);
 for(band=0; band <7; band++)
 {
 digitalWrite(strobe,LOW); // strobe pin on the shield - kicks the IC up to the next band 
 delayMicroseconds(30); // 
 eqLevels[band] = analogRead(EQ_OUT1); // store left band reading
 digitalWrite(strobe,HIGH); 
 }
 dumpEQ();
}

void dumpEQ(){

// display values of right channel on serial monitor
 for (band = 0; band < 7; band++)
 {
 Serial.print(eqLevels[band]);
 Serial.print(" ");
 }
 Serial.println();
 
}


  

