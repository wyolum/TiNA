#include <Wire.h>
#include <SD.h>

#define BUTTONPIN A6 // uses a resistor ladder for decoding 5 buttons

const int WID = 42;
// const int DS3231_ADDR = 104; // define in RTC code
const int N_DATA_BYTE = 32;
const int BAUD = 9600;
const int CHIPSELECT = 10;
const int strobe = A0; // strobe pins on digital 4
const int res = A1; // reset pins on digital 5


uint8_t data[N_DATA_BYTE];
uint8_t address;

void setup(){
  Serial.begin(BAUD);
  pinMode(13, OUTPUT);
  pinMode(BUTTONPIN, INPUT); 
  digitalWrite(BUTTONPIN, HIGH); 
  pinMode(res, OUTPUT); // reset
  pinMode(strobe, OUTPUT); // strobe

  Serial.println("Hello");
  data[0] = 255; // reserved
  data[1] = 255; // SD
  data[2] = 0; // LED
  if (test_SD() == 0)
    Serial.println("SD passed");
  else
    Serial.println("SD failed");
  testRTC();
  delay(1000);
}

void loop(){
  while(Serial.available()){
    // Serial.write(Serial.read()); // ECHO SERIAL
  }
  // Button test is  a little flakey, may need other resistor values
  // testButtons();
  delay(50);
  testMSGEQ7();

}

void testButtons(){
  int reading = analogRead(BUTTONPIN);
/*
   Serial.println(reading);
  delay(500);
  return;
*/
  Serial.print(reading);
  Serial.print(" ");
  if (reading > 900)
    Serial.println("    up");
  else if (reading > 700){
    Serial.println("    down");
  }
  else if (reading > 500){
    Serial.println("    middle");
  }
  else if (reading > 300){
    Serial.println("    left");
  }
  else if (reading > 100){
    Serial.println("    right");
  }
  else{
    Serial.println("");
  }
 //else   Serial.println(reading);
}

uint8_t test_SD(){
  uint8_t status = 0; // ALL PASS
  File myFile;
  char *msg = "0123456789";

  pinMode(CHIPSELECT, OUTPUT);
  if (!SD.begin(CHIPSELECT)) {
    status |= 1 << 2;
  }
  else{
    SD.remove("test.txt");
    myFile = SD.open("test.txt", FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile) {
      delay(300);
      myFile.print(msg);
      myFile.close();
    } 
    else {
      // if the file didn't open for writing, print an error:
      status |= 1 << 1;
    }

    // re-open the file for reading:
    myFile = SD.open("test.txt");
    if (myFile) {
      int ii = 0;
      
      while (myFile.available() && ii < 5) {
	if(msg[ii++] != myFile.read()){
	  status |= 1;
	}
      }
      myFile.close();
    }
  }
  return status;
}

// Example 48.1 - tronixstuff.com/tutorials > chapter 48 - 30 Jan 2013 
// MSGEQ7 spectrum analyser shield - basic demonstration
int left[7]; // store band values in these arrays
int right[7];
int band;
int EQ_OUT1 = A2;
int EQ_OUT2 = A3;
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
 left[band] = analogRead(EQ_OUT1); // store left band reading
 right[band] = analogRead(EQ_OUT2); // ... and the right
 digitalWrite(strobe,HIGH); 
 }
}
void testMSGEQ7()
{
  InitMSGEq7();
 readMSGEQ7();
 
 // display values of left channel on serial monitor
 for (band = 0; band < 7; band++)
 {
   //Serial.print(left[band]);
   //Serial.print(" ");
 }
 // Serial.println();

// display values of right channel on serial monitor
 for (band = 0; band < 7; band++)
 {
 Serial.print(right[band]);
 Serial.print(" ");
 }
 Serial.println();
 
}
/********************************************************************************
 * RTC code
 ********************************************************************************/
#define IS_BCD true
#define IS_DEC false
#define IS_BYTES false
const int DS3231_ADDR = 104;

// decimal to binary coded decimal
uint8_t dec2bcd(int dec){
  uint8_t t = dec / 10;
  uint8_t o = dec - t * 10;
  return (t << 4) + o;
}

// binary coded decimal to decimal
int bcd2dec(uint8_t bcd){
  return (((bcd & 0b11110000)>>4)*10 + (bcd & 0b00001111));
}


bool rtc_raw_read(uint8_t addr,
		  uint8_t n_bytes,
		  bool is_bcd,
		  uint8_t *dest){

  bool out = false;
  Wire.beginTransmission(DS3231_ADDR); 
  // Wire.send(addr); 
  Wire.write(addr);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_ADDR, (int)n_bytes); // request n_bytes bytes 
  if(Wire.available()){
    for(uint8_t i = 0; i < n_bytes; i++){
      dest[i] = Wire.read();
      if(is_bcd){ // needs to be converted to dec
	dest[i] = bcd2dec(dest[i]);
      }
    }
    out = true;
  }
  return out;
}

bool testRTC(){
  bool status = true;
  uint8_t date[7];
  if(rtc_raw_read(0, 7, true, date)){
    Serial.print("DATE: ");
    // date[2], date[1], date[0], date[4], date[5], date[6]
    //      hr,     min,     sec,     day,   month,  yr;
      Serial.print(date[2], DEC);
      Serial.print(":");
      Serial.print(date[1], DEC);
      Serial.print(":");
      Serial.print(date[0], DEC);
      Serial.print("  ");

      Serial.print(date[4], DEC);
      Serial.print("/");
      Serial.print(date[5], DEC);
      Serial.print("/");
      Serial.print(date[6], DEC);
      Serial.print(".");

    Serial.println("");
  }
  else{
    Serial.print("RTC FAIL");
    status = false;
  }
  return status;
}
/********************************************************************************
 * END RTC code
 ********************************************************************************/
