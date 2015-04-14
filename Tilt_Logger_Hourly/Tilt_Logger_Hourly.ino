/*
	Tilt_Logger_Hourly

	This sketch logs data from an attached tilt meter and GPS unit. Log
        files change every hour and are stamped per the convention below.
        The wildfire board was used for its two serial ports and watchdog timer
        capability. The GPS comes from the Adafruit Ultimate GPS logger shield. 
        We use the SD card on that shield as well, just because of the way things
        mount in the housing. See the repository readme for more hardware description.

	The circuit:
	* list the components attached to each input
	* list the components attached to each output

	Created 14 04 2015
	By John R. Leeman
	Modified 14 04 2015
	By John R. Leeman

	https://github.com/jrleeman/WildfireTiltLogger

*/

#include <WildFire.h>
#include <SPI.h>
#include <SD.h>
#include <TinyGPS.h>

uint8_t index = 0;
// Holds current hour, 99 forces us to start a new log at first GPS lock
int current_hour = 99; 
char tiltBuffer[100];
boolean writeTilt = false; // State variable for when we are ready to log

File logfile;
TinyGPS gps;
WildFire wildfire;

void setup() {
  wildfire.begin();

  // Start up the serial port 
  Serial.begin(9600);
  Serial1.begin(9600);
  
  // Setup the tilt meter
  delay(3000);
  int bytesSent = Serial.write("*9900XY-SET-BAUDRATE,9600\n"); // Set baudrate on tilt meter
  delay(1000);
  bytesSent += Serial.write("*9900SO-SIM\n"); // Set to NMEA XDR format
  delay(1000);
  bytesSent += Serial.write("*9900XYC2\n"); // Set to 1 Hz output
  delay(5000);
  
  // TODO: Add GPS setup verification here
  
  // Setup and start the SD card
  pinMode(10, OUTPUT);
  SD.begin(10, 11, 12, 13);
  delay(1000);
  
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
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  // Done with reading the GPS 
    
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
    float flat, flon;
    unsigned long age;
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
    gps.f_get_position(&flat, &flon, &age);
    
    // Check and deal with the need for a new logging file
    if (hour != current_hour){
      logfile.close();
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
    
    char sz[32];
    sprintf(sz, "%04d-%02d-%02dT%02d:%02d:%02d,%02d,",
        year, month, day, hour, minute, second, gps.satellites());
    logfile.write(sz);
    log_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
    logfile.write(",");
    log_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);
    logfile.write(",");
    log_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 7, 2);
    logfile.write(tiltBuffer);
    logfile.flush();
    writeTilt = false;
  }  
  
}

static void log_float(float val, float invalid, int len, int prec)
{
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
 /*
 Given a day, month, and year (2 or 4 digit), returns
 the day of year. Errors return 999.
 */
 // List of days in a month for a normal year
 int daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};

 // Check if it is a leap year
 if (year%4  == 0)
 {
   daysInMonth[1] = 29;
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
