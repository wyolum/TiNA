#include <SD.h>
#include "font.h"

uint8_t font_read_char(File font_file, char c, uint8_t *dest){
  uint8_t n_byte;
  font_file.seek(c * FONT_RECLEN);
  n_byte = (uint8_t)font_file.read();

  for(uint8_t i = 0; i < n_byte; i++){
    dest[i] = (uint8_t)font_file.read();
  }
  return n_byte;
}

bool char_is_blank(char c, File font_file){
  bool out = true;
  uint8_t n_byte;
  uint8_t i;
  uint8_t font_data[FONT_RECLEN - 1];


  n_byte = font_read_char(font_file, c, font_data);
  for(i = 0; i < n_byte && out; i++){
    if(font_data[i] > 0){
      out = false;
    }
  }
  return out;
}
