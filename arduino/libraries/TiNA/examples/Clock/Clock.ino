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
uint32_t last_time = 0;
bool update_required = true;
uint32_t start_time = 0;
uint32_t next_time;
uint8_t digits[10*4] = {62,65,65,62, // 0
			0,66,127,64, // 1
			98,81,73,70, // 2
			34,73,73,54, // 3_MODE
			30,16,16,127,// 4
			39,69,69,57, // 5
			62,73,73,50, // 6
			97,17,9,7,   // 7
			54,73,73,54, // 8
			38,73,73,62}; // 9

uint8_t r_bitmap[32];
uint8_t g_bitmap[32];
uint8_t b_bitmap[32];

const uint8_t MODE_CLOCK = 0;
const uint8_t MODE_HH = 1;
const uint8_t MODE_MM = 2;
const uint8_t MODE_SS = 3;
const uint8_t MODE_RACE = 4;
const uint8_t N_MODE = 5;
uint8_t mode = MODE_CLOCK;

uint8_t event = BUTTON_NONE;

void bitmap_clear(){
  for(uint8_t i = 0; i < 32; i++){
    r_bitmap[i] = 0;
    g_bitmap[i] = 0;
    b_bitmap[i] = 0;
  }
}

void bitmap_setpixel(uint8_t x, uint8_t y, uint8_t *bitmap, bool val){
  if(val){
    bitmap[x] |= ((uint8_t)1) << y;
  }
  else{
    // bitmap[x] |= ((uint8_t)val) << y;
  }
}

void rgb_setpixel(uint8_t x, uint8_t y, uint32_t color){
  uint8_t r, g, b;
  tina.color2rgb(color, &r, &g, &b);
  bitmap_setpixel(x, y, r_bitmap, r > 0);
  bitmap_setpixel(x, y, g_bitmap, g > 0);
  bitmap_setpixel(x, y, b_bitmap, b > 0);
}

void set4x7_digit(uint8_t x, uint8_t y, uint8_t digit, uint32_t color){
  uint8_t val;
  uint8_t bit;

  for(uint8_t i=x; i<x + 4; i++){
    val = digits[4 * digit + i - x];
    //Serial.println(val);
    for(uint8_t j = 0; j < 7; j++){
      bit = (val >> j) & 1;
      rgb_setpixel(i, j + y, color * bit);
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
  setSyncProvider(getTime);      // RTC
  setSyncInterval(60000);      // update every minute (and on boot)

  tina.clear();
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

void display_time(uint32_t t, uint32_t color, bool leading_zero_f){
  if(t != last_time || update_required){
    last_time = t;
    update_required = true;

    hh = (t / 3600) % 12;
    mm = (t / 60) % 60;
    ss = t % 60;

    bitmap_clear();
    two_digits(0, hh, color, leading_zero_f);
    two_digits(11, mm, color, true);
    two_digits(22, ss, color, true);

    if(ss % 2){
      colen(10, tina.Color(25, 25, 25));
      colen(21, tina.Color(25, 25, 25));
    }
    else{
      colen(9, tina.Color(25, 25, 25));
      colen(20, tina.Color(25, 25, 25));
    }  
  }
}

// display rgb image stored in global r_, g_ and b_bitmap variables
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

uint32_t count = 0;
uint32_t last_update = 0;
uint32_t brightness = tina.Color(25, 25, 25);

void clock_loop(){
  display_time(next_time, tina.Color(0, 0, 25), false);
  if(update_required){
    Serial.println("UPDATE");
    display_bitmap(brightness);
    update_required = false;
  }
  if(event == BUTTON_UP){
    start_time = getTime() + 10;
    mode = MODE_RACE;
  }
  if(event == BUTTON_LEFT){
    brightness += tina.Color(5, 5, 5);
    update_required = true;
  }
  else if(event == BUTTON_RIGHT){
    brightness -= tina.Color(5, 5, 5);
    update_required = true;
  }
  else if(event == BUTTON_MIDDLE){
    mode = MODE_HH;
  }
}

void race_loop(){
  if(start_time > getTime()){
    display_time(start_time - next_time, tina.Color(25, 0, 0), true);
  }
  else{
    display_time(next_time - start_time, tina.Color(0, 25, 0), true);
  }
  if(update_required){
    Serial.println("UPDATE");
    display_bitmap(brightness);
    update_required = false;
  }

  if(event == BUTTON_LEFT){
    brightness += tina.Color(5, 5, 5);
    update_required = true;
  }
  else if(event == BUTTON_RIGHT){
    brightness -= tina.Color(5, 5, 5);
    update_required = true;
  }
}

void set_hh_loop(){
  display_time(next_time, tina.Color(25, 0, 0), false);
  for(uint8_t ii = 2; ii < 8; ii++){
    rgb_setpixel(ii, 0, tina.Color(1, 0, 0));
  }
  display_bitmap(brightness);

  if(event == BUTTON_MIDDLE){
    mode = MODE_CLOCK;
  }
  else if(event == BUTTON_UP){
    mode = MODE_MM;
  }
  else if(event == BUTTON_DOWN){
    mode = MODE_SS;
  }
  else if(event == BUTTON_LEFT){
    setRTC(getTime() + 3600);
  }
  else if(event == BUTTON_RIGHT){
    setRTC(getTime() - 3600);
  }
}
void set_mm_loop(){
  display_time(next_time, tina.Color(25, 0, 0), false);
  for(uint8_t ii = 12;  ii < 18; ii++){
    rgb_setpixel(ii, 0, tina.Color(1, 0, 0));
  }
  display_bitmap(brightness);
  if(event == BUTTON_MIDDLE){
    mode = MODE_CLOCK;
  }
  else if(event == BUTTON_UP){
    mode = MODE_SS;
  }
  else if(event == BUTTON_DOWN){
    mode = MODE_HH;
  }
  else if(event == BUTTON_LEFT){
    setRTC(getTime() + 60);
  }
  else if(event == BUTTON_RIGHT){
    setRTC(getTime() - 60);
  }
}
void set_ss_loop(){
  uint32_t temp_time;

  display_time(next_time, tina.Color(25, 0, 0), false);
  for(uint8_t ii = 23; ii < 29; ii++){
    rgb_setpixel(ii, 0, tina.Color(1, 0, 0));
  }
  display_bitmap(brightness);
  if(event == BUTTON_MIDDLE){
    mode = MODE_CLOCK;
  }
  else if(event == BUTTON_UP){
    mode = MODE_HH;
  }
  else if(event == BUTTON_DOWN){
    mode = MODE_MM;
  }
  else if(event == BUTTON_LEFT || event == BUTTON_RIGHT){
    temp_time = getTime();
    temp_time -= temp_time % 60;
    setRTC(temp_time);
  }
}

void loop(){
  uint8_t next_second;
  
  next_time = getTime(); // now() might jump a fraction of a second here or there, getTime() goes direct to the DS3231
  if(mode == MODE_CLOCK){
    clock_loop();
  }
  else if(mode == MODE_HH){
    set_hh_loop();
  }
  else if(mode == MODE_MM){
    set_mm_loop();
  }
  else if(mode == MODE_SS){
    set_ss_loop();
  }
  else if(mode == MODE_RACE){
    race_loop();
  }

  event = BUTTON_NONE; // *_loop must handle all events that it is interested in

  interact();
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
  rgb_setpixel(x, 3, color);
  rgb_setpixel(x, 5, color);
}

uint8_t interact(){
  uint32_t start_ms = millis();
  uint8_t button = tina.getButton();
  uint32_t min_hold = 100; // debounce
  
  if(event == button){
    // dont repeat pending events
  }
  else{
    while(button && tina.getButton() == button){
      if(millis() - start_ms > min_hold){
	event = button;
	rgb_setpixel(0, 0, tina.Color(1, 1, 1));
	display_bitmap(brightness);
      }
    }
    rgb_setpixel(0, 0, 0);
    display_bitmap(brightness);
  }

  return event;
}
