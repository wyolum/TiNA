/*--------------------------------------------------------------------
  
  --------------------------------------------------------------------*/

#include "TiNA.h"
TiNA::TiNA(){
}
bool TiNA::setup(int _n_tim){
  setup(_n_tim, false);
}
bool TiNA::setup(int _n_tim, bool use_sd){
  bool status = true;
  n_tim = _n_tim;
  n_led_per_strip = n_tim * 16;
  n_byte_per_strip = n_led_per_strip * N_BYTE_PER_LED;

  pinMode(LED_PIN, INPUT);
  digitalWrite(LED_PIN, LOW); 
  
  pinMode(BUTTONPIN, INPUT); 
  digitalWrite(BUTTONPIN, HIGH) ;

  if(!use_sd){
    for(uint8_t i = 0; i < N_STRIP; i++){
      // all strips share the same buffer
      strips[i].setup(n_led_per_strip, pins[i], NEO_GRB + NEO_KHZ800, buffer);
    }
    error_code = TINA_OK;
  }
  else if (!SD.begin(SD_CS)) {
    error_code = TINA_SD_INIT_ERROR;
    status = false;
  }
  else{
    font_file = SD.open("ASCII5X7.WFF");
    if(!font_file){
      error_code = TINA_FONT_OPEN_ERROR;
      status = false;
    }
    else{

      // buffer up 10 ditigs
      for(int i=0; i < 10; i++){
	font_read_char(font_file, '0' + i, digits + i * (FONT_RECLEN - 1));
      }

      display_file = SD.open("_DSP", FILE_WRITE);
      if(!display_file){
	error_code = TINA_DSP_OPEN_ERROR;
	status = false;
      }
      else{
	for(uint8_t i = 0; i < N_STRIP; i++){
	  // all strips share the same buffer
	  strips[i].setup(n_led_per_strip, pins[i], NEO_GRB + NEO_KHZ800, buffer);
	}
	clear();
	initialized = true;
	error_code = TINA_OK;
      }
    }
  }
  return status;
}
void TiNA::error(int _error_code){
  error_code = error_code;
}
void TiNA::clear(){
  fill(0);
}
void TiNA::color2rgb(uint32_t color, uint8_t *r, uint8_t *g, uint8_t *b){
  *g = (color >> 16) & 0xff;
  *r = (color >>  8) & 0xff;
  *b = (color >>  0) & 0xff;
}

void TiNA::fill(uint32_t color){
  uint8_t r, g, b;
  display_file.seek(0);
  color2rgb(color, &r, &g, &b);
  for(uint8_t j = 0; j < N_STRIP; j++){
    for(uint8_t i = 0; i < n_led_per_strip; i++){
      display_file.write(r);
      display_file.write(g);
      display_file.write(b);
    }
  }
}

uint32_t TiNA::Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)g << 16) | ((uint32_t)r <<  8) | b;
}

void TiNA::setpixel(uint16_t x, uint16_t y, uint32_t color){
  // x is col, y is row
  uint8_t r, g, b;
  if(y < 8 && x < n_led_per_strip){
    color2rgb(color, &r, &g, &b);
    display_file.seek(y * n_byte_per_strip + x * N_BYTE_PER_LED);
    display_file.write(g);
    display_file.write(r);
    display_file.write(b);
    strips[y].changed = true;
  }
}
void TiNA::show(){
  display_file.seek(0);
  for(uint8_t i = 0; i < N_STRIP; i++){
    display_file.read(strips[i].pixels, n_byte_per_strip);
    strips[i].changed = true;
    strips[i].show();
  }
}
uint16_t TiNA::put_char(uint16_t x, char c, uint32_t color){
  uint8_t font_data[FONT_RECLEN - 1];
  if('0' <= c && c <= '9'){ // read from buffer
    for(int i = 0; i < FONT_RECLEN - 1; i++){
      font_data[i] = digits[(c - '0') * (FONT_RECLEN - 1) + i];
    } 
  }
  else{
    font_read_char(tina.font_file, c, font_data);
  }
  for(int i=0; i < FONT_RECLEN - 1; i++){
    for(int j=0; j < 7; j++){
      if((font_data[i] >> j) & 1){
	setpixel(x + i, j + 1, color);
      }
      else{
	setpixel(x + i, j + 1, 0);
      }
    }
  }
  return FONT_RECLEN - 1;
}

uint8_t TiNA::getButton(){
  int reading = analogRead(BUTTONPIN);
  uint8_t out = BUTTON_NONE;
  if (reading > 900){
    out = BUTTON_UP;
  }
  else if (reading > 700){
    out = BUTTON_DOWN;
  }
  else if (reading > 500){
    out = BUTTON_MIDDLE;
  }
  else if (reading > 300){
    out = BUTTON_LEFT;
  }
  else if (reading > 100){
    out = BUTTON_RIGHT;
  }
  else{
    out = BUTTON_NONE;
  }
  return out;
}

