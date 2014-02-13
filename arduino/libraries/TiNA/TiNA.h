// Copyright 2013 WyoLum, LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied.  See the License for the specific language
// governing permissions and limitations under the License.

#ifndef TINA_H
#define TINA_H
#include <inttypes.h>
#include "SD.h"
#include "font.h"
#include "Adafruit_NeoPixel.h"

// set HW_VERSION to 0 when following EPD schematic
#define HW_VERSION1

const int SD_CS = 10; 
const uint8_t N_STRIP = 8;
const uint8_t N_BYTE_PER_LED = 3;

const uint8_t TINA_OK = 0;
const uint8_t TINA_SD_INIT_ERROR = 1;
const uint8_t TINA_FONT_OPEN_ERROR = 2;
const uint8_t TINA_DSP_OPEN_ERROR = 3;

const uint8_t BUTTONPIN = A6;
const uint8_t BUTTON_NONE = 0;
const uint8_t BUTTON_UP = 1;
const uint8_t BUTTON_DOWN = 2;
const uint8_t BUTTON_MIDDLE = 3;
const uint8_t BUTTON_LEFT = 4;
const uint8_t BUTTON_RIGHT = 5;

// error codes
const uint8_t SD_ERROR_CODE = 0;
const uint8_t FILE_NOT_FOUND_CODE = 1;
// Use Arduino Pins 2-9 for controling the strips of TiM
// const uint8_t pins[8] = {2, 3, 4, 5, 6, 7, 8, 9};
const uint8_t pins[8] = {9, 8, 7, 6, 5, 4, 3, 2};

class TiNA{
 public:
  bool initialized;
  File display_file;
  File font_file;
  uint8_t font_data[FONT_RECLEN - 1];
  uint16_t n_led_per_strip;
  uint16_t n_byte_per_strip;
  Adafruit_NeoPixel strips[N_STRIP];
  uint8_t n_tim;
  uint8_t buffer[N_BYTE_PER_LED * 16 * 3]; // MAX of 3 TiMs, 16 leds per TiM
  uint16_t error_code;
  uint8_t digits[50]; // buffer digits for clocks
  
  // constructor
  TiNA();
  void error(int code_num);

  // call in arduino setup function
  bool setup(int n_tim);
  bool setup(int n_tim, bool use_sd);

  // clear the display
  void clear();

  // fill the display with a single color
  void fill(uint32_t color);
  
  uint32_t Color(uint8_t r, uint8_t g, uint8_t b);
  void color2rgb(uint32_t color, uint8_t *r, uint8_t *g, uint8_t *b);
  // set a pixel to a value
  void setpixel(uint16_t x, uint16_t y, uint32_t color);
  void show();
  uint16_t put_char(uint16_t x, char c, uint32_t color);
  bool char_is_blank(char c);
  uint8_t getButton();
};

// define the E-Ink display
extern TiNA tina;
#endif
