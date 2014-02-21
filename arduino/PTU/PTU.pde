// HERE!
#include "rtcBOB.h"
#include "Time.h"
#include "Wire.h"

#include "Arduino.h"
#include "SoftwareSerial.h"

#include <TinyGPS++.h>
#include <EEPROM.h>

#define DEFINED

volatile unsigned long count = 0;
volatile unsigned long pps_start_us = 0;
volatile unsigned long pps_tick_us = 1000000;  // filtered
volatile unsigned long _pps_tick_us = 1000000; // unfiltered

volatile unsigned long rtc_start_us = 0;          
volatile unsigned long rtc_tick_us = 1000000;  // filtered
volatile unsigned long _rtc_tick_us = 1000000; // unfiltered

unsigned long sync_time = 0;     // seconds of last GPS 1pps sync
unsigned long last_set_time = 0; // seconds of last GPS time set

// drift = actual_us - desired_us
// drift > 0 ==> pulse is late
// drift < 0 ==> pulse is early
volatile          long rtc_drift_us;          // Filtered


volatile boolean pps_led_state = true;
volatile boolean sqw_led_state = true;
volatile boolean synced = false;

const int SQW_PIN = 3;
const int PPS_PIN = 2;
const int  PPS_LED = 7;
const int  SQW_LED = 4;
const unsigned int RTC_STALE_S = 1800;
const unsigned int RTC_SECOND_STALE_S = 1000 * 3600;// 1000 hours
const unsigned long  RTC_WRITE_US = 1750;
const unsigned long OFFSET_US = 1000;
const uint8_t chipSelect = 10;
const unsigned long DELTA_TOL = 5000;

void pps_interrupt(){
  unsigned long  now_us = micros();
  _pps_tick_us = (now_us - pps_start_us);
  pps_led_state = !pps_led_state;
  digitalWrite(PPS_LED, pps_led_state);
  pps_start_us = now_us;
}

void rtc_interrupt(){
  unsigned long  now_us = micros();
  long drift_us;
  // Serial.println("rtc_interrupt");
  sqw_led_state = !sqw_led_state;
  digitalWrite(SQW_LED, sqw_led_state);
  rtc_start_us = now_us;
}

bool save_sync_time(){
  uint8_t len = 14;

  char dat[len];
  dat[1] = len;
  Time_to_Serial(sync_time, dat + 2);
  Time_to_Serial(last_set_time, dat + 6);
  ulong_to_Serial(pps_tick_us, dat + 10); 
  ulong_to_Serial(rtc_tick_us, dat + 14);
}

bool restore_sync_time(){
  bool out;
  uint8_t len = 14;
  char dat2[len];

  if(out){
    sync_time = Serial_to_time(dat2 + 2);
    last_set_time = Serial_to_time(dat2 + 4);
  }
  return out;
}

// write 4 bytes of in into char buffer out.
void Time_to_Serial(time_t in, char *out){
  time_t *out_p = (time_t *)out;
  *out_p = in;
}

time_t Serial_to_time(char *in){
  time_t out;
  out = *(time_t *)in;
  return out;
}

// write 4 bytes of in into char buffer out.
void ulong_to_Serial(unsigned long in, char *out){
  unsigned long *out_p = (unsigned long *)out;
  *out_p = in;
}
unsigned long Serial_to_ulong(char *in){
  unsigned long out;
  out = *(unsigned long *)in;
  return out;
}

static const int RXPin = 6, TXPin = A7;
static const uint32_t GPSBaud = 9600;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial sws(RXPin, TXPin);

void setup()
{
  Serial.begin(115200);
  sws.begin(GPSBaud);

  Serial.begin(115200);
  Serial.println("Precision Timing Unit v1.0");
  Serial.println("Copyright WyoLum, LLC, 2014");

  Wire.begin();
  // From MaceTech.com: set 1Hz reference square wave
  Wire.beginTransmission(0x68); // address DS3231
  WIRE_WRITE1(0x0E); // select register
  WIRE_WRITE1(0b00000000); // write register bitmap, bit 7 is /EOSC
  Wire.endTransmission();

  pinMode(PPS_LED, OUTPUT);
  pinMode(SQW_LED, OUTPUT);

  pinMode(PPS_PIN, INPUT);
  pinMode(SQW_PIN, INPUT);

  digitalWrite(PPS_LED, HIGH);
  digitalWrite(SQW_LED, HIGH);

  delay(100);
  digitalWrite(PPS_LED, LOW);
  digitalWrite(SQW_LED, LOW);

  set_1Hz_ref(getTime(), SQW_PIN, rtc_interrupt, FALLING);
  Serial.println("Waiting for 1Hz signal");
  while(rtc_start_us == 0){
    delay(100);
  }
  Serial.println("1Hz signal present");
  attachInterrupt(PPS_PIN - 2, pps_interrupt, RISING);
  smartDelay(1000);
  if(micros() - pps_start_us < 2e6){
    Serial.println("1pps signal present.");
  }
  else{
    Serial.println("NO 1pps signal present");
  }
  
  Serial.print("micros() - pps_start_us: ");
  Serial.println(micros() - pps_start_us);

  setRTC(2010, 1, 1, 0, 0, 0); // DBG!!! TODO:

  initialize_clock();

  Serial.print("now():");
  Serial.println(now());

  Serial.println("initialization done.");
}

void initialize_clock(){

  if((year() < 2011 or year() > 2040)){ // first acq
    Serial.print("Year:");
    Serial.print(year());
    Serial.println("< 2011");
    sws.begin(9600);
    while(year() < 2011){
      smartDelay(1000);
      Serial.print("gps.time.age(): ");
      Serial.println(gps.time.age());
      if (gps.time.age() < 1000 && micros() - pps_start_us < 1e6){
	setRTC(gps.date.year(), 
	       gps.date.month(),
	       gps.date.day(), 
	       gps.time.hour(), 
	       gps.time.minute(), 
	       gps.time.second());
	Serial.println(year());
	setTime(getTime());
      }
      Serial.print(gps.date.year());
      Serial.print(" ");
      Serial.print(year());
      Serial.print(" ");
      Serial.println(gps.date.age());
    }
    sws.end();
    attachInterrupt(SQW_PIN - 2, pps_interrupt, RISING);
    Serial.print("new year:");
    Serial.println(year());
  } // expect YMD HM to be current beyond here.
  seconds_sync();
  fine_sync();
}

// assumes HHMMDD HHMM are correct
void seconds_sync(){
  unsigned long start_s;
  if(now() - last_set_time > RTC_SECOND_STALE_S){
    sws.begin(9600);
    pause_1Hz();
    start_s = now();

    // wait for next second boundary
    while(now() == start_s){
      smartDelay(0);
    }
    sws.end();
    unpause_1Hz();
    last_set_time = now();
    save_sync_time();
  }
}

// assumes YY/MM/DD are accurate
/*
 * TJS: If RTC is not stale, do nothing.  Otherwise do fine sync with GPS.
 */
void fine_sync(){
  boolean out = false;
  unsigned long now_s;
  unsigned long target_us;
  unsigned long delta_us;
  int sec_frac;
  
  if((micros() - pps_start_us) < 2 * pps_tick_us){ // fine sync available
    Serial.println("Fine sync available");
    if(pps_start_us > rtc_start_us){
      delta_us = pps_start_us - rtc_start_us;
    }
    else{
      delta_us = rtc_start_us - pps_start_us;
    }
    // sync if time is stale and we have a GPS signal
    if((now() - sync_time > RTC_STALE_S && (micros() - pps_start_us) < 2 * pps_tick_us) || 
       delta_us > DELTA_TOL){
      Serial.println("fine_sync()");
      Serial.print(now());
      Serial.print("-");
      Serial.print(sync_time);
      Serial.print(" ");
      Serial.println(now() - sync_time);
      now_s = now();
      Serial.println("Time stale");

      // have YYMMDD hhmmss, now synchronize pulses
      // 1. let tick times settle
      for(int i = 0; i < 3; i++){
	wait_for_next_gps_second();
	Serial.println("gps tick()");
      }
      target_us = 0;
      while(target_us < pps_start_us){ // watch for integer overflow
	// 2. wait for start of next second
	wait_for_next_gps_second();
	target_us = pps_start_us + pps_tick_us;
      }
      // 3. compute next second
      now_s = now();
      sec_frac = millisecond();
      if(sec_frac > 500){
	now_s++;
      }
    
      // 3. pause clock
      pause_1Hz();

      // 4. wait for expected next rtc pulse
      // RTC_WRITE_US is the time it takes to set clock
      // OFFSET_US is to keep the interrupts from colliding.
      while(micros() < target_us - RTC_WRITE_US + OFFSET_US){
      }
      setRTC(now_s + 1);

      // 5. unpause 1Hz
      unpause_1Hz();

      // 6 check sync
      for(int i = 0; i < 3; i++){
	wait_for_next_gps_second();
	Serial.println("gps tick()");
      }
      sqw_led_state = !pps_led_state;
      Serial.print("SYNC RESULT:");
      Serial.print(rtc_start_us);
      Serial.print(", ");
      Serial.print(pps_start_us);
      Serial.print(", ");
      if(pps_start_us < rtc_start_us){
	Serial.println(rtc_start_us - pps_start_us);
      }
      else{
	Serial.print("-");
	Serial.println(pps_start_us - rtc_start_us);
      }
      sync_time = now();
      save_sync_time();
    }
  }
  else{
    Serial.println("Fine sync NOT available");
  }
}

void wait_for_next_rtc_second(){
  while(digitalRead(SQW_PIN) == LOW){}
  while(digitalRead(SQW_PIN) == HIGH){}
}
void wait_for_next_gps_second(){
  while(digitalRead(PPS_PIN) == HIGH){}
  while(digitalRead(PPS_PIN) == LOW){}
}

void grab_datetime(unsigned long _date, 
		   unsigned long _time,
		   long lat,
		   long lon,
		   long alt,
		   unsigned long speed,
		   unsigned long course){
  int YY, MM, DD, hh, mm, ss;
  unsigned long date = _date;
  unsigned long time = _time;
  hh = time / 1000000;
  time = time % 1000000;
  mm = time / 10000;
  time = time % 10000;
  ss = time / 100;
  setRTC(year(), month(), day(), hh, mm, ss);
  setTime(getTime());
  // Serial.print("LAT:");
  // Serial.println(lat / 1e5);
  // Serial.print("LON:");
  // Serial.println(lon / 1e5);
  if(date > 0){
    DD = date / 10000;
    date = date % 10000;
    MM = date / 100; 
    date = date % 100;
    YY = 2000 + date;
    setRTC(YY, MM, DD, hour(), minute(), second());
  }

  last_set_time = getTime();
  setTime(last_set_time);

#ifdef NOT_DEFINED
  Serial.print("YY");
  Serial.println(YY);
  Serial.print("year():");
  Serial.println(year());
  Serial.print(_date);
  Serial.print(",");
  Serial.print(_time);
  Serial.print("  ");
  Serial.print(YY);
  Serial.print("/");
  Serial.print(MM);
  Serial.print("/");
  Serial.print(DD);
  Serial.print(" ");
  Serial.print(hh);
  Serial.print(":");
  Serial.print(mm);
  Serial.print(":");
  Serial.print(ss);
  Serial.println();
  setRTC(year(), month(), day(), hour(), minute(), second());
  Serial.print(YY);
  Serial.print("/");
  Serial.print(MM);
  Serial.print("/");
  Serial.println(DD);
#endif
}

void loop(){
  Serial.print("loop() ");
  Serial.print(rtc_start_us);
  Serial.print(", ");
  Serial.print(pps_start_us);
  Serial.print(", ");
  if(pps_start_us < rtc_start_us){
    Serial.print(rtc_start_us - pps_start_us);
    if((rtc_start_us - pps_start_us) < 2000){
      // good to go
    }
    else{
      // resync
    }
  }
  else{
    Serial.print("-");
    Serial.print(pps_start_us - rtc_start_us);
  }
  Serial.print(" uS offset, ");
  Serial.print(year());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.print(second());
  Serial.println();

  fine_sync();
  delay(10000);
}

void do_nothing(){
}
bool feedgps()
{
  while (sws.available())
  {
    char c = sws.read();
    // Serial.print(c);
    if (gps.encode(c))
      return true;
  }
  return false;
}

void printFloat(double number, int digits)
{
  // Handle negative numbers
  if (number < 0.0)
  {
     Serial.print('-');
     number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i=0; i<digits; ++i)
    rounding /= 10.0;
  
  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  Serial.print(int_part);

  // Print the decimal point, but only if there are digits beyond
  if (digits > 0)
    Serial.print("."); 

  // Extract digits from the remainder one at a time
  while (digits-- > 0)
  {
    remainder *= 10.0;
    int toPrint = int(remainder);
    Serial.print(toPrint);
    remainder -= toPrint; 
  } 
}
// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (sws.available())
      gps.encode(sws.read());
  } while (millis() - start < ms);
}

