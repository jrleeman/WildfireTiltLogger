#include <WildFire.h>
#include <SPI.h>
#include <SD.h>

File logfile;

char tiltBuffer[100];
uint8_t index = 0;
WildFire wildfire;
boolean writeTilt = false;

void setup() {
  wildfire.begin();
  
  Serial.begin(9600);
  Serial1.begin(9600);
  
  pinMode(10, OUTPUT);
  SD.begin(10, 11, 12, 13);
  
  logfile = SD.open("LOG.TXT", FILE_WRITE);
  
  delay(1000);

}

void loop() {
  bool newData = false;
  
  // Grab any serial from the Tiltmeter and store it
  while (Serial.available() > 0) {
    if (index < 100) {
      char inChar = Serial.read();
      tiltBuffer[index] = inChar;
      index++;
      if (inChar == '\n'){
        writeTilt = true;
        index = 0;
      }
    }
  }
  // Done with getting any tilt meter data!
  
  
  // Log the tiltmeter data if we are ready
  if (writeTilt){
    logfile.write(tiltBuffer);
    logfile.flush();
    writeTilt = false;
  }  
  
}
