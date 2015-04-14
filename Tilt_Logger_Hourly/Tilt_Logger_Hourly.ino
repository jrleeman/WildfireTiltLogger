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

int current_hour = 99;

void setup() {
  wildfire.begin();
  
  Serial.begin(9600);
  Serial1.begin(9600);
  
  delay(3000);
  int bytesSent = Serial.write("*9900XY-SET-BAUDRATE,9600\n"); // Set baudrate on tilt meter
  delay(1000);
  bytesSent += Serial.write("*9900SO-SO\n"); // Set to NMEA XDR format
  delay(1000);
  bytesSent += Serial.write("*9900XYC2\n"); // Set to 1 Hz output
  delay(2000);
  
  pinMode(10, OUTPUT);
  SD.begin(10, 11, 12, 13);
  
  logfile = SD.open("LOG.TXT", FILE_WRITE);
  
  logfile.write("YYYY-MM-DDTHH:MM:SS,NUM_SATELLITES,LATITUDE,LONGITUDE,ALTITUDE,X_TILT,Y_TILT,TEMPERATURE,SERIAL_NUMBER,EVT_MARKER\n");
  
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
      if (inChar == '$')
        inChar = ',';
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
      logfile.write("YYYY-MM-DDTHH:MM:SS,NUM_SATELLITES,LATITUDE,LONGITUDE,ALTITUDE,X_TILT,Y_TILT,TEMPERATURE,SERIAL_NUMBER,EVT_MARKER\n");
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
