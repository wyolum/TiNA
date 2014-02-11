#include <SD.h>
#include <Wire.h>
#include <Time.h>
#include "TiNA.h"
#include "font.h"
#include "Adafruit_NeoPixel.h"

TiNA tina;

void setup(){
  Serial.begin(115200);
  if(!tina.setup(3)){
    Serial.print("TiNA setup failed.  Error code:");
    Serial.println(tina.error_code);
  }
  else{
    Serial.println("TiNA setup ok");
  }
  tina.fill(tina.Color(0, 0, 0));
  tina.show();
}

void loop(){
  int row, col, n;
  uint8_t font_data[FONT_RECLEN];
  for(int i=0; i < FONT_RECLEN; i++){
    font_data[i] = 0;
  }

  tina.put_char(0, 'A', tina.Color(0, 50, 0));
  tina.put_char(5, 'B',  tina.Color(0, 0, 50));
  tina.put_char(10, 'C', tina.Color(50, 0, 0));
  tina.put_char(15, '1', tina.Color(0, 50, 0));
  tina.put_char(20, '2', tina.Color(0, 0, 50));
  tina.put_char(25, '3', tina.Color(50, 0, 0));
  tina.put_char(30, '4', tina.Color(0, 50, 0));
  tina.show();
  while(1) delay(100);
}
