void setup() {
  Serial.begin(9600);
}

void loop() {

Serial.println("$YXXDR,A,803.924,M,N,A,-2264.226,D,E,C,17.09,C,T-N1052*40");
delay(1000);
Serial.println("$YXXDR,A,804.063,M,N,A,-2264.189,D,E,C,17.09,C,T-N1052*4B");
delay(1000);
Serial.println("$YXXDR,A,803.863,M,N,A,-2264.164,D,E,C,17.07,C,T-N1052*49");
delay(1000);
Serial.println("$YXXDR,A,803.790,M,N,A,-2264.290,D,E,C,17.08,C,T-N1052*4D");
delay(1000);
Serial.println("$YXXDR,A,803.824,M,N,A,-2264.142,D,E,C,17.07,C,T-N1052*4E");
delay(1000);

}
