#include <WildFire.h>
#include <SPI.h>
#include <SD.h>

File logfile;

char gpsBuffer[200];
uint8_t index = 0;
WildFire wildfire;
boolean writeGPS = false;

void setup() {
  wildfire.begin();
  
  Serial.begin(9600);

  
  pinMode(10, OUTPUT);
  SD.begin(10, 11, 12, 13);
  
  logfile = SD.open("LOG.TXT", FILE_WRITE);
  
  Serial1.begin(9600);
  delay(1000);
  Serial.println("Starting GPS Logging!");

}

void loop() {
  
  // Grab any serial from the Tiltmeter and store it
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    Serial.print(c);
    if (index < 200) {
      gpsBuffer[index] = c;
      index++;
    }
    if (c == '\n'){
      writeGPS = true;
      index = 0;
    }
  }
  // Done with getting any tilt meter data!
  
  // Log the GPS data if we are ready
  if (writeGPS){
    Serial.println(gpsBuffer);
    logfile.write(gpsBuffer);
    logfile.flush();
    writeGPS = false;
  }  
  
  
}
