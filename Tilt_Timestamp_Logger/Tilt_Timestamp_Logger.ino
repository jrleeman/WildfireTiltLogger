#include <WildFire.h>
#include <SPI.h>
#include <SD.h>
#include <TinyGPS.h>

File logfile;

char tiltBuffer[100];
uint8_t index = 0;
WildFire wildfire;
boolean writeTilt = false;
TinyGPS gps;

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
   while (Serial1.available())
    {
      char c = Serial1.read();
      // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
    
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
    int year;
    byte month, day, hour, minute, second, hundredths;
    unsigned long age;
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d,%02d:%02d:%02d,",
        month, day, year, hour, minute, second);
    logfile.write(sz);
    logfile.write(tiltBuffer);
    logfile.flush();
    writeTilt = false;
  }  
  
}
