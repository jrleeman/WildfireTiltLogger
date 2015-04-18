/*
	Tilt_Logger_Hourly

	This sketch logs data from an attached tilt meter and GPS unit. Log
        files change every hour and are stamped per the convention below.
        The wildfire board was used for its two serial ports and watchdog timer
        capability. The GPS comes from the Adafruit Ultimate GPS logger shield. 
        We use the SD card on that shield as well, just because of the way things
        mount in the housing. See the repository readme for more hardware description.

	Created 14 04 2015
	By John R. Leeman
	Modified 14 04 2015
	By John R. Leeman

	https://github.com/jrleeman/WildfireTiltLogger

*/

// DEV NOTES:
// - Master works and is logging as of about noon April 14
// - This code has not been tested, but shouldn't cause any 
//   major issues. Maybe just by putting the file logger
//   new file in a function we could break.Maybe I need to figure out
//   how to return the file object? Testing will tell. 
// - The GPS setup stuff should be tested, it didn't seem to work
//   a few nights ago when I was playing with it.

#include <WildFire.h>
#include <SPI.h>
#include <SD.h>
#include <TinyGPS.h>

uint8_t index = 0; // Buffer index
uint8_t ngps = 0; // nubmer GPS sentences
int current_hour = 99; // 99 forces us to start a new log at first GPS lock
char tiltBuffer[100];
boolean writeTilt = false; // State variable for when we are ready to log
int current_second = 99;
int current_day = 999;
boolean getTilt = false;

File logfile;
TinyGPS gps;
WildFire wildfire;

int year;
byte month, day, hour, minute, second, hundredths;
unsigned long age;

void setup() {
  wildfire.begin();

  // Start up the serial port 
  Serial.begin(9600);
  Serial1.begin(9600);
  
  // Setup the tilt meter
  delay(3000);
  int bytesSent = Serial.println("*9900SE"); // Disable Echo
  delay(1000);
  bytesSent = Serial.println("*9900TR-SP"); // Disable power on message
  delay(1000);
  bytesSent = Serial.println("*9900XY-SET-BAUDRATE,9600"); // Set baudrate on tilt meter
  delay(1000);
  bytesSent += Serial.println("*9900SO-SIM"); // Set to NMEA XDR format
  delay(1000);
  bytesSent += Serial.println("*9900XYC-OFF"); // Set call/response mode
  delay(5000);
  
  // Setup the GPS Parameters
  Serial1.println("$PMTK251,9600*17"); // 9600 Baud
  delay(1000);
  Serial1.println("$PMTK220,1000*1F"); // 1Hz Update
  delay(1000);
  Serial1.println("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"); // GPRMC and GGA
  delay(1000);
  Serial1.println("$PGCMD,33,0*6D"); // No Antenna State Updates Please
  delay(1000);
  
  // Setup and start the SD card
  pinMode(10, OUTPUT);
  SD.begin(10, 11, 12, 13);
  delay(1000);
  
    // Dump the serial buffer to trash all of the crap the 
  // tilt meter tells us about our commands and such.
  while (Serial1.available() > 0) {
    Serial1.read();
  }

  // Dump the serial buffer to trash all of the crap the 
  // tilt meter tells us about our commands and such.
  while (Serial.available() > 0) {
    Serial.read();
  }
}

void loop() {
  
  // Read a GPS sentence if there is one avaliable
  bool newData = false;
   while (Serial1.available())
    {
      char c = Serial1.read();
      
      // Did a new valid sentence come in?
      if (gps.encode(c)) { 
        ngps +=1;
      }
      if (ngps == 2){  
        ngps = 0;
        // Open up the date and time. See if the second has changed since the last valid message.
        // If so, let's go get new tilt measurements!
        gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
        if (second != current_second){
          newData = true;
          current_second = second;
          current_day = day;
        }
      }
    }
  // Done with reading the GPS 
  
  // If there is a new GPS sentence of a different second, get the tilt meter data
  if (newData) {
    
    // SEND CALL
    Serial.println("*9900XY"); // Set call/response mode
    Serial.flush();
    delay(500); // Give the tilt meter a chance to start responding, 800 causes skips sometimes 
    // Grab any serial from the Tiltmeter and store it, while we are
    // at it, we'll dump the $ and spaces that are in the output
    while (Serial.available() > 0) {
      if (index < 100) {
        char inChar = Serial.read();
      
        if (inChar == '$')
          inChar = ',';
       
        if (inChar != ' '){
          tiltBuffer[index] = inChar;
          index++;
        }
      }
    }
    index = 0;
    // Done with getting any tilt meter data!
  
  
    // Grab the latest gps data and parse it up for writing to the log file,
    // this should be the most recent time stamp at the time of the tilt reading.
    float flat, flon;
    gps.f_get_position(&flat, &flon, &age);
    
    // Check and deal with the need for a new logging file
    if (hour != current_hour){
      start_new_logfile(day, month, year, hour);
    }
    
    // Format and write out the GPS data
    char sz[32];
    sprintf(sz, "%04d-%02d-%02dT%02d:%02d:%02d,%02d,",
        year, month, day, hour, minute, second, gps.satellites());
    logfile.write(sz);
    log_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
    logfile.write(",");
    log_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);
    logfile.write(",");
    log_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 7, 2);
    
    // Write out the tilt data and flush the write buffer
    logfile.write(tiltBuffer);
    logfile.flush();  
  }
}

void start_new_logfile(int day, int month, int year, int hour){
  logfile.close(); // Close the old logfile
  char name[13];
  getLogFileName(name,day,month,year,hour);
  
  // Prevent over-writing by adding to the sub-sequence set
  for (uint8_t i = 0; i < 10; i++) {
    name[7] = '0' + i;
    if (! SD.exists(name)) {
      break;
    }
  }
  
  // Open the new file and set the current hour to the new hour
  logfile = SD.open(name, FILE_WRITE);
  if( ! logfile ) {
    //error(1);
    int dummy = 0;
  }
  current_hour = hour;
  logfile.write("YYYY-MM-DDTHH:MM:SS,NUM_SATELLITES,LATITUDE,LONGITUDE,ALTITUDE,X_TILT,Y_TILT,TEMPERATURE,SERIAL_NUMBER\n");
}

static void log_float(float val, float invalid, int len, int prec) {
  /*
  Logs a float value
  */
  if (val == invalid)
  {
    while (len-- > 1)
      logfile.write('*');
  }
  else
  {
    logfile.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
  }
}

char getLogFileName(char *filename, int day, int month, int year, int hour) {
  /*
  Format is DDDYYHHA.DAT
  DDD - Day of year
  YY  - Year
  NN  - Logger unit number
  A - Sub-sequence name, used if more than one file/day
  */
  strcpy(filename, "");
  strcpy(filename, "00000000.TXT");
  
  // Get day of year and set it
  int doy = calculateDayOfYear(day, month, year);
  filename[0] = '0' + (doy/100)%100;
  filename[1] = '0' + (doy/10)%10;
  filename[2] = '0' + doy%10;
  
  // Set the year
  filename[3] = '0' + (year/10)%10;
  filename[4] = '0' + year%10;
  
  // Set the hour
  if (hour != 0){
    filename[5] = '0' + (hour/10)%10;
    filename[6] = '0' + hour%10;
  }
}

int calculateDayOfYear(int day, int month, int year) {
  // Given a day, month, and year (4 digit), returns 
  // the day of year. Errors return 999.
  
  int daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  
  // Verify we got a 4-digit year
  if (year < 1000) {
    return 999;
  }
  
  // Check if it is a leap year, this is confusing business
  // See: https://support.microsoft.com/en-us/kb/214019
  if (year%4  == 0) {
    if (year%100 != 0) {
      daysInMonth[1] = 29;
    }
    else {
      if (year%400 == 0) {
        daysInMonth[1] = 29;
      }
    }
   }

  // Make sure we are on a valid day of the month
  if (day < 1) 
  {
    return 999;
  } else if (day > daysInMonth[month-1]) {
    return 999;
  }
  
  int doy = 0;
  for (int i = 0; i < month - 1; i++) {
    doy += daysInMonth[i];
  }
  
  doy += day;
  return doy;
}
