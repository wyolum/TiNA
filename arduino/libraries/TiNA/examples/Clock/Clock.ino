#include <SD.h>
#include <Wire.h>
#include <Time.h>
#include "TiNA.h"
#include "font.h"
#include "Adafruit_NeoPixel.h"
#include "rtcBOB.h"

TiNA tina;

uint16_t YY;
uint8_t MM;
uint8_t DD;
uint8_t hh;
uint8_t mm;
uint8_t ss;
uint32_t start_time = 0;
uint8_t digits[10*4] = {62,65,65,62, // 0
			0,66,127,64, // 1
			98,81,73,70, // 2
			34,73,73,54, // 3
			30,16,16,127,// 4
			39,69,69,57, // 5
			62,73,73,50, // 6
			97,17,9,7,   // 7
			54,73,73,54, // 8
			38,73,73,62}; // 9

uint8_t r_bitmap[32];
uint8_t g_bitmap[32];
uint8_t b_bitmap[32];

void set4x7_digit(uint8_t x, uint8_t y, uint8_t digit, uint32_t color){
  uint8_t val;
  for(uint8_t i=x; i<x + 4; i++){
    val = digits[4 * digit + i - x];
    Serial.println(val);
    for(uint8_t j=y; j < y + 7; j++){
      if((val >> (j - y)) & 1){
	tina.setpixel(i, j, color);
      }
      else{
	tina.setpixel(i, j, 0);
      }
    }
  }
}

void setup(){
  Serial.begin(115200);
  if(!tina.setup(3)){
    Serial.print("TiNA setup failed.  Error code:");
    Serial.println(tina.error_code);
  }
  else{
    Serial.println("TiNA setup ok");
  }
  Wire.begin();
  // Serial.println("Wire started");
  // Serial.println(getTime());
  setSyncProvider(getTime);      // RTC
  setSyncInterval(60000);      // update every minute (and on boot)
  // update_time();

  tina.fill(tina.Color(0, 0, 0));
  tina.show();
}

void two_digits(uint8_t x, uint8_t val, uint32_t color, bool leading_zero_f){
  if(val / 10 || leading_zero_f){
    set4x7_digit(x, 1, val/10, color);
  }
  else{
    set4x7_digit(x, 1, 0, 0);
  }
  set4x7_digit(x + 5, 1, val%10, color);
}

void display_time(uint32_t t, uint32_t hh_color, uint32_t mm_color, uint32_t ss_color, bool leading_zero_f){
  hh = (t / 3600) % 12;
  mm = (t / 60) % 60;
  ss = t % 60;

  two_digits(0, hh, hh_color, leading_zero_f);
  two_digits(11, mm, mm_color, true);
  two_digits(22, ss, ss_color, true);

  if(ss % 2){
    colen(10, tina.Color(25, 25, 25));
    colen(21, tina.Color(25, 25, 25));
    colen(9, 0);
    colen(20, 0);
  }
  else{
    colen(10, 0);
    colen(21, 0);
    colen(9, tina.Color(25, 25, 25));
    colen(20, tina.Color(25, 25, 25));
  }  
}

uint32_t count = 0;
uint32_t last_update = 0;
void clock_loop(){
  uint32_t color;
  uint32_t next_time;
  uint8_t next_second;
  bool countdown_mode;

  // dim colens between hh, mm, and ss
  next_time = getTime() + 1; // now() might jump a fraction of a second here or there, getTime() goes direct to the DS3231
  next_second = next_time % 60;

  if(start_time < next_time){
    next_time -= start_time;
    color = tina.Color(0, 50, 0);
    countdown_mode = false;
  }
  else{
    next_time = start_time - next_time;
    color = tina.Color(50, 0, 0);
    countdown_mode = true;
  }
  display_time(next_time, color, color, color, false);
  while(getTime() % 60 != next_second){
  }
  tina.show();
  last_update = millis();
  count++;
}

// display mono image stored in global bitmap variable
void display_bitmap(uint32_t color){
  uint8_t r, g, b;
  tina.color2rgb(color, &r, &g, &b);
  for(uint8_t j = 0; j < 8; j++){
    for(uint8_t i = 0; i < 32; i++){
      tina.strips[j].pixels[3 * i + 1] = r * (r_bitmap[i] >> j & 1);
      tina.strips[j].pixels[3 * i + 0] = g * (g_bitmap[i] >> j & 1);
      tina.strips[j].pixels[3 * i + 2] = b * (b_bitmap[i] >> j & 1);
    }
    tina.strips[j].changed = true;
    tina.strips[j].show();
  }
}

void set_hh_loop(){
  uint32_t next_second = getTime() % 60 + 1;
  if(tina.getButton() == BUTTON_UP){
    setRTC(getTime() + 3600);
  }
  if(tina.getButton() == BUTTON_DOWN){
    setRTC(getTime() - 3600);
  }
  display_time(getTime(), tina.Color(25, 25, 25), tina.Color(0, 25, 0), tina.Color(0, 25, 0), false);
  tina.show();
}
void set_mm_loop(){
}
void set_ss_loop(){
}

void loop(){
  // set_hh_loop();
  for(int i = 0; i < 32; i++){
    r_bitmap[i] = i + count;
    g_bitmap[i] = i - count;
    b_bitmap[i] = i + 2 * count;
  }
  display_bitmap(tina.Color(0, 25, 0));
  count++;
  return;
  clock_loop();
  // while(1) delay(100);
  if(start_time == 0 && tina.getButton()){
    start_time = getTime() + 10;
  }
}

/* add the "1" char at the very front 
   not enough room for a full font "1"
*/
void addone(uint32_t color){
  for(int i = 1; i < 8; i++){
    tina.setpixel(1, i, color);
  }
  tina.setpixel(0, 7, color);
  tina.setpixel(2, 7, color);
  tina.setpixel(0, 2, color);
}

void colen(uint8_t x, uint32_t color){
  tina.setpixel(x, 3, color);
  tina.setpixel(x, 5, color);
}

void interact(){
  uint32_t start_ms;
  uint8_t button = tina.getButton();
  uint32_t min_hold = 1000;

  if(button == BUTTON_MIDDLE){
    start_ms = millis();
    while((tina.getButton() == button) && (millis() - start_ms < min_hold)){
      // wait for min hold duration
    }
    if(millis() - start_ms > min_hold){
      while(1) delay(100);
    }
  }
}
