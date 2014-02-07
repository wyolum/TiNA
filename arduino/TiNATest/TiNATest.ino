#include <Wire.h>
#include <SD.h>

#define BUTTONPIN A6 // uses a resistor ladder for decoding 5 buttons

const int WID = 42;
const int DS3231_ADDR = 104;
const int N_DATA_BYTE = 32;
const int BAUD = 9600;
const int CHIPSELECT = 10;
uint8_t data[N_DATA_BYTE];
uint8_t address;

void setup(){
  Serial.begin(BAUD);
  Wire.begin(WID);
  pinMode(13, OUTPUT);
  pinMode(BUTTONPIN, INPUT); 
  digitalWrite(BUTTONPIN, HIGH); 

  Serial.println("Hello");
  data[0] = 255; // reserved
  data[1] = 255; // SD
  data[2] = 0; // LED
  if (test_SD() == 0)
    Serial.println("SD passed");
  else
    Serial.println("SD failed");
}

void loop(){
  while(Serial.available()){
    Serial.write(Serial.read()); // ECHO SERIAL
  }
  // Button test is  a little flakey, may need other resistor values
  //testButtons();
//  delay(500);
  testMSGEQ7();

}

void testButtons(){
  int reading = analogRead(BUTTONPIN);
/*
   Serial.println(reading);
  delay(500);
  return;
*/
  if ((reading >380)&& (reading < 450))
    Serial.println("left");
  else if (reading == 203)
    Serial.println("right");
  else if (reading > 1000)
    Serial.println("up");
  else if ( reading > 780 && reading < 850)
    Serial.println("down");
  else if ( reading > 590 && reading < 625)
    Serial.println("middle");
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
int strobe = A0; // strobe pins on digital 4
int res = A1; // reset pins on digital 5
int left[7]; // store band values in these arrays
int right[7];
int band;
int EQ_OUT1 = A2;
int EQ_OUT2 = A3;
void InitMSGEq7()
{
 pinMode(res, OUTPUT); // reset
 pinMode(strobe, OUTPUT); // strobe
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
 readMSGEQ7();
 
 // display values of left channel on serial monitor
 for (band = 0; band < 7; band++)
 {
 Serial.print(left[band]);
 Serial.print(" ");
 }
 Serial.println();

// display values of right channel on serial monitor
 for (band = 0; band < 7; band++)
 {
 Serial.print(right[band]);
 Serial.print(" ");
 }
 Serial.println();
 
}
