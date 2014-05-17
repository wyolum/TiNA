#include <SD.h>
#include <Wire.h>
#include <Time.h>
#include "TiNA.h"
#include "font.h"
#include "Adafruit_NeoPixel.h"
#include "rtcBOB.h"

TiNA tina;

bool flip = true; // default orientation, change to false if you assemble differently

// Time parameters
uint16_t YY;
uint8_t MM;
uint8_t DD;
uint8_t hh;
uint8_t mm;
uint8_t ss;
uint32_t last_time = 0;

// set this to true whenever you change the display and would like an immediate refresh
bool update_required = true;

// this gets set to the start time of the race.
uint32_t start_time = 0;

// The next time to be displayed
uint32_t next_time;

// font for numbers.  Figured out on spreadsheet.
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

// bitmap for each color.  no color depth here, just on or off to save memory
uint8_t r_bitmap[32];
uint8_t g_bitmap[32];
uint8_t b_bitmap[32];

//Different mode constants.  The master loop() uses these to direct execution.
const uint8_t MODE_CLOCK = 0;
const uint8_t MODE_HH = 1;
const uint8_t MODE_MM = 2;
const uint8_t MODE_SS = 3;
const uint8_t MODE_RACE = 4;
const uint8_t N_MODE = 5;

const uint32_t   RED = tina.Color(1, 0, 0);
const uint32_t GREEN = tina.Color(0, 1, 0);
const uint32_t  BLUE = tina.Color(0, 0, 1);
const uint32_t WHITE = tina.Color(1, 1, 1);


// Some timing constants
const uint32_t DEBOUNCE_MS = 100;     // a button must be depressed for this many miliseconds befor it "counts"
const uint32_t LONG_HOLD_MS = 1000;   // This defines how long a "long hold" is.  
                                      //     Used to escape from race mode and you don't want to do it by accedent 
const uint32_t RACE_COUNTDOWN_S = 10; // Countdown in seconds

uint8_t mode = MODE_CLOCK;      // stored the current mode with MODE_CLOCK as the default
uint8_t event = BUTTON_NONE;    // stores any button presses
uint32_t min_hold = DEBOUNCE_MS;// min amount of time for a button press to count.
uint32_t brightness = tina.Color(8, 8, 8); // 0, 0, 0 = off, 255, 255, 255 = BRIGHT
// [0, 1, 2, 4, 8, 16, 32, 64, 128, 255] valid brightness values

// Serial Messaging
/********************************************************************************/
/*
 * This is a input/output buffer for serial communications.  
 * This buffer is also used for some non-serial processing to save memory.
 */
const uint32_t SERIAL_TIMEOUT_MS = 100;
const uint8_t MAX_MSG_LEN = 100; // official: 100
char serial_msg[MAX_MSG_LEN];
uint8_t serial_msg_len;       // Stores actual length of serial message.  Should be updated upon transmit or reciept.
const uint8_t SYNC_BYTE = 254;   // 0xEF; (does not seem to work!)
uint8_t sync_msg_byte_counter = 0;  // How many byte in a row has sync byte been recieved 

typedef void (* CallBackPtr)(); // this is a typedef for callback funtions

struct MsgDef{
  uint8_t id;     // message type id

  /* n_byte holde the number of bytes
   * with message length in second byte of message. */
  uint8_t n_byte; 

  /* Function to call when this message type is recieved.
   * message content (header byte removed) will available to the call back function through
   * the global variabe "char serial_msg[]".
   */
  CallBackPtr cb; 
};


// Message types
const MsgDef  NOT_USED_MSG   = {0x00, 1, do_nothing};
const MsgDef  ABS_TIME_REQ   = {0x01, 1, send_time};
const MsgDef  ABS_TIME_SET   = {0x02, 5, Serial_time_set};
const MsgDef  TRIGGER_BUTTON = {0x03, 2, button_interrupt};
const MsgDef          PING   = {0x04, MAX_MSG_LEN, pong};
const uint8_t N_MSG_TYPE     = 0x05;

// array to loop over when new messages arrive.
const MsgDef *MSG_DEFS[N_MSG_TYPE] = {&NOT_USED_MSG,
				      &ABS_TIME_REQ,
				      &ABS_TIME_SET,
				      &TRIGGER_BUTTON,
				      &PING
				      };

void do_nothing(){
}

// write 4 bytes of in into char buffer out.
void Time_to_Serial(time_t in, char *out){
  time_t *out_p = (time_t *)out;
  *out_p = in;
}


void send_time(){
  char ser_data[4];
  Serial.write(ABS_TIME_SET.id);
  Time_to_Serial(now() - start_time, ser_data);
  for(uint8_t i = 0; i < 4; i++){
    Serial.write(ser_data[i]);
  }
}

void Serial_time_set(){
  setRTC(Serial_to_Time(serial_msg));
}

// in is the address of the start of 4 time bytes.
time_t Serial_to_Time(char *in){
  return *((uint32_t *)in);
}

// simulate a button press
void button_interrupt(){
  event = serial_msg[0];
}

void pong(){
  for(uint8_t i=0; i < MAX_MSG_LEN - 1; i++){
    Serial.write(serial_msg[i]);
  }
}


// store serial data into serial_msg staring from first byte AFTER MID
// to be clear MID is not in serial_msg
boolean Serial_get_msg(uint8_t n_byte, uint32_t timeout_ms) {
  /*
    n_byte = message length including 1 byte MID
  */
  uint16_t i = 0;
  unsigned long start_time = millis();

  uint8_t val, next;
  boolean out;
  uint32_t start = millis();

  while((i < n_byte - 1) && ((millis() - start_time) < timeout_ms)){
    if(Serial.available()){
      val = Serial.read();
      if (val == SYNC_BYTE){
	sync_msg_byte_counter++;
	if(sync_msg_byte_counter == MAX_MSG_LEN){
	   // no other valid msg combination can have MAX_MSG_LEN sync bytes.
	   // sync msg recieved! break out, next char is start of new message
	   sync_msg_byte_counter = 0;
	   break;
	 }
       }
       else{
	 sync_msg_byte_counter = 0;
       }
       serial_msg[i++] = val;
     }
   }
   if(i == n_byte - 1){
     digitalWrite(LED_PIN, LOW);
     out = true;
   }
   else{
    digitalWrite(LED_PIN, HIGH);
    out = false;
  }
  return out;
}


void Serial_loop(void) {
  uint8_t val;
  boolean resync_flag = true;
  if(Serial.available()){
    val = Serial.read();
    for(uint8_t msg_i = 0; msg_i < N_MSG_TYPE; msg_i++){
      if(MSG_DEFS[msg_i]->id == val){
	if(Serial_get_msg(MSG_DEFS[msg_i]->n_byte, SERIAL_TIMEOUT_MS)){
	  /*
	   * Entire payload (n_byte - 1) bytes 
	   * is stored in serial_msg: callback time.
	   */

	  MSG_DEFS[msg_i]->cb();
	  //two_digits(val);
	  //c3.refresh(10000);
	}
	else{
	  // Got a sync message unexpectedly. Get ready for new message.
	  // no callback
	  // or timeout
	}
	resync_flag = false;
	break;
	// return;
      }
    }
    if(resync_flag){
      Serial_sync_wait();
    }
  }
}
void Serial_sync_wait(){
  // wait for SYNC message;
  uint8_t val;
  uint8_t n = 0;
  digitalWrite(LED_PIN, HIGH);
  while(n < MAX_MSG_LEN){
    if(Serial.available()){
      val = Serial.read();
      if(val == SYNC_BYTE){
	n++;
      }
      else{
	n = 0;
      }
    }
  }
  digitalWrite(LED_PIN, LOW);
}

// end serial library
/********************************************************************************/

/*
 * clear R, G, and B bitmaps
 */
void bitmap_clear(){
  for(uint8_t i = 0; i < 32; i++){
    r_bitmap[i] = 0;
    g_bitmap[i] = 0;
    b_bitmap[i] = 0;
  }
}

/*
 * set a pixel on or off for specified single color bitmap
 */
void bitmap_setpixel(uint8_t x, uint8_t y, uint8_t *bitmap, bool val){
  if(x < 32 && y < 8){
    if(val){
      bitmap[x] |= ((uint8_t)1) << y;
    }
    else{
      // bitmap[x] |= ((uint8_t)val) << y;
    }
  }
}

/*
 * set rgb pixel
 */
void rgb_setpixel(int x, int y, uint32_t color){
  uint8_t r, g, b;
  tina.color2rgb(color, &r, &g, &b);
  bitmap_setpixel(x, y, r_bitmap, r > 0);
  bitmap_setpixel(x, y, g_bitmap, g > 0);
  bitmap_setpixel(x, y, b_bitmap, b > 0);
}

/*
 * set digit in specified place with specified color
 */
void set4x7_digit(int x, int y, uint8_t digit, uint32_t color){
  uint8_t val;
  uint8_t bit;

  for(int i=x; i<x + 4; i++){
    val = digits[4 * digit + i - x];
    //Serial.println(val);
    for(uint8_t j = 0; j < 7; j++){
      bit = (val >> j) & 1;
      rgb_setpixel(i, j + y, color * bit);
    }
  }
}

/*
 * Code starts here on boot up.  This only gets called once.
 */
void setup(){
  Serial.begin(115200);
  if(!tina.setup(4)){
    // Serial.print("TiNA setup failed.  Error code:");
    // Serial.println(tina.error_code);
  }
  else{
    // Serial.println("TiNA setup ok");
  }
  Wire.begin();
  setSyncProvider(getTime);      // RTC
  setSyncInterval(60000);      // update every minute (and on boot)

  tina.clear();
}

/*
 * set two digits to specified location and color
 */
void two_digits(int x, uint8_t val, uint32_t color, bool leading_zero_f){
  if(val / 10 || leading_zero_f){
    set4x7_digit(x, 1, val/10, color);
  }
  else{
    set4x7_digit(x, 1, 0, 0);
  }
  set4x7_digit(x + 5, 1, val%10, color);
}

/*
 * set display buffer (do not push out to pixels)
 */
void display_time(uint32_t t, uint32_t color, bool leading_zero_f){
  if(t != last_time || update_required){
    last_time = t;
    update_required = true;

    hh = (t / 3600) % 12;
    if (hh == 0 && mode != MODE_RACE){
      hh = 12;
    }
    mm = (t / 60) % 60;
    ss = t % 60;

    bitmap_clear();
    two_digits(-1, hh, color, leading_zero_f);
    two_digits(11, mm, color, true);
    two_digits(23, ss, color, true);

    uint32_t colen_color = color;
    colen(9, colen_color);
    colen(21, colen_color);
  }
}

/* 
 * push buffer out to pixels
 * display rgb image stored in global r_, g_ and b_bitmap variables
 */
void display_bitmap(uint32_t color){
  uint8_t r, g, b;
  tina.color2rgb(color, &r, &g, &b);
  uint8_t ii; // used for flip
  uint8_t jj; // used for flip
  
  for(uint8_t j = 0; j < 8; j++){
    if(flip){
      jj = 7 - j;
    }
    else{
      jj = j;  
    }
    for(uint8_t i = 0; i < 32; i++){
      if(flip){
        ii = 31 - i;
      }
      else{
        ii = i;
      }
      tina.strips[jj].pixels[3 * ii + 1] = r * (r_bitmap[i] >> j & 1);
      tina.strips[jj].pixels[3 * ii + 0] = g * (g_bitmap[i] >> j & 1);
      tina.strips[jj].pixels[3 * ii + 2] = b * (b_bitmap[i] >> j & 1);

      // get back side too
      tina.strips[jj].pixels[3 * ii + 1 + 32 * 3] = r * (r_bitmap[i] >> j & 1);
      tina.strips[jj].pixels[3 * ii + 0 + 32 * 3] = g * (g_bitmap[i] >> j & 1);
      tina.strips[jj].pixels[3 * ii + 2 + 32 * 3] = b * (b_bitmap[i] >> j & 1);
    }
    tina.strips[jj].changed = true;
    tina.strips[jj].show();
  }
}

/*
 * brighten the display
 */ 
void brighten(){
  if(brightness % 256 == 0){
    brightness = tina.Color(1, 1, 1);
  }
  else if(brightness % 256 <= 127){
    brightness *= 2;
  }
  else{
    brightness = tina.Color(255, 255, 255);
  }
  update_required = true;
}
void dim(){
  if(brightness % 256 <= 1){
    brightness = tina.Color(0, 0, 0);
  }
  else  if(brightness % 256 <= 128){
    brightness /= 2;
  }
  else{
    brightness = tina.Color(128, 128, 128); // off
  }
  update_required = true;
}

/*
 * This loop gets called several times a second when in clock mode.
 */ 
void clock_loop(){
  display_time(next_time, BLUE, false);
  if(update_required){
    //Serial.println("UPDATE");
    display_bitmap(brightness);
    update_required = false;
  }
  if(event == BUTTON_UP){ // start the race
    // rgb_setpixel(0, 0, 1);  display_bitmap(brightness);  while(1) delay(100); /// DBG LINE
    start_time = getTime() + RACE_COUNTDOWN_S;
    mode = MODE_RACE;
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_MIDDLE){
    mode = MODE_HH;
    display_time(next_time, RED, false);
    update_required = true;
    display_bitmap(brightness);
	event = BUTTON_NONE;
  }
}

/*
 * called several times per second during race mode
 */
void race_loop(){
  if(start_time > getTime()){ // display countdown in RED
    display_time(start_time - next_time, RED, false);
  }
  else{ // display normal race time in GREEN
    display_time(next_time - start_time, GREEN, false);
  }
  if(update_required){
    //Serial.println("UPDATE");
    display_bitmap(brightness);
    update_required = false;
  }

  if(button_pressed(BUTTON_MIDDLE, LONG_HOLD_MS)){ // require long press to get out of race mode (loss of race timing)
	// cancel race_loop
	mode = MODE_CLOCK;
  }
}

/*
 *  Set the HH
 */
void set_hh_loop(){
  display_time(next_time, RED, false);
  for(uint8_t ii = 0; ii < 8; ii++){
    rgb_setpixel(ii, 0, RED);
  }
  display_bitmap(brightness);

  if(event == BUTTON_MIDDLE){
    mode = MODE_CLOCK;
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_DOWN){
    mode = MODE_MM;
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_UP){
    mode = MODE_SS;
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_LEFT){
    setRTC(getTime() + 3600);
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_RIGHT){
    setRTC(getTime() - 3600);
	event = BUTTON_NONE;
  }
}
/*
 *  Set the MM
 */
void set_mm_loop(){
  display_time(next_time, RED, false);
  for(uint8_t ii = 11;  ii < 20; ii++){
    rgb_setpixel(ii, 0, RED);
  }
  display_bitmap(brightness);
  if(event == BUTTON_MIDDLE){
	event = BUTTON_NONE;
    mode = MODE_CLOCK;
  }
  else if(event == BUTTON_DOWN){
    mode = MODE_SS;
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_UP){
    mode = MODE_HH;
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_LEFT){
    setRTC(getTime() + 60);
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_RIGHT){
    setRTC(getTime() - 60);
	event = BUTTON_NONE;
  }
}

/*
 *  Set the SS
 */
void set_ss_loop(){
  uint32_t temp_time;

  display_time(next_time, RED, false);
  for(uint8_t ii = 23; ii < 32; ii++){
    rgb_setpixel(ii, 0, RED);
  }
  display_bitmap(brightness);
  if(event == BUTTON_MIDDLE){
    mode = MODE_CLOCK;
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_DOWN){
    mode = MODE_HH;
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_UP){
    mode = MODE_MM;
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_LEFT || event == BUTTON_RIGHT){
    temp_time = getTime();
    temp_time -= temp_time % 60;
    setRTC(temp_time);
	event = BUTTON_NONE;
  }
}

/*
 * This is the main loop, gets called as fast as possible.
 * farms out real work to other *_loop() functions depending on mode.
 */
void loop(){  
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

  // default button handlers
  if(event == BUTTON_LEFT){ // change this to any button you want to brighten the display
    brighten();
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_RIGHT){ // same here
    dim();
	event = BUTTON_NONE;
  }
  else if(event == BUTTON_DOWN){ // flip display
    flip = !flip; 
    update_required = true;
	event = BUTTON_NONE;
  }

  event = BUTTON_NONE; // all events are cleared after each loop.

  interact(min_hold);
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

/*
 * add a colen (two dots) at specified x location
 */
void colen(uint8_t x, uint32_t color){
  rgb_setpixel(x, 3, color);
  rgb_setpixel(x, 5, color);
}

/*
 * detect any button pushes
 */
uint8_t interact(uint32_t min_hold){
  uint32_t start_ms = millis();
  uint8_t button = tina.getButton();
  //uint32_t min_hold = 100; // debounce
  
  Serial_loop(); // sets event if a serial event has occured
  if(event == button){
    // dont repeat pending events
  }
  else{
    while(button && tina.getButton() == button){
      if(millis() - start_ms > min_hold){
        event = button;
	    //rgb_setpixel(0, 0, WHITE);
	    //display_bitmap(brightness);
      }
    }
    //rgb_setpixel(0, 0, 0);
    //display_bitmap(brightness);
  }

  return event;
}

/*
 * return true if button is pressed for at least minimum hold min_hold.
 */
bool button_pressed(uint8_t button, uint32_t min_hold){
	uint32_t start = millis();
	if(tina.getButton() == button){
		while((tina.getButton() == button)){
		}
	}
	return (millis() - start) > min_hold;
}
