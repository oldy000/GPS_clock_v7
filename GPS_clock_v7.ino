// things to add to this project
// 6. adjustable display brightness, potentially varies with ambient light level.
// 8. push button to get the current age
// put code and instruction into github so button functions can be found


// include libraries
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "Statistic.h"  // without trailing s


// define Arduino Nano pins
static const int RXPin = 4, TXPin = 3;  //gps receiver is pin D4
static const uint32_t GPSBaud = 9600;

// pin 0 for receiving from s7s isnt actually used
static const int s7sRXPin = 0, s7sTXPin = 2;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

// The serial connection to the s7s device
SoftwareSerial s7s(s7sRXPin, s7sTXPin);
unsigned int counter = 0;  // This variable will count up to 65k
char tempString[10];  // Will be used with sprintf to create strings

// define the digital pin for the speaker signal
int speakerPin = 3;

unsigned long BEEP_TIMER = 0;

unsigned long nowmillis;
unsigned long lastmillis = 0;
unsigned long secos;          //this is millis() / 1000, i.e seconds since program started running.
unsigned long power_on;       // predicted time in seconds that nano was powered on. Will be between 0 and 86400
int numreadings = 300;
Statistic myStats;
boolean power_on_lock = false;
unsigned long power_on_final;
float sd_threshold = 2.0;
unsigned long nano_time;
boolean nano_time_valid = true;

long validity_score = 0;
int LAST_GPS_SECOND;

// time zone definition
// Perth, Western Australia is UTC+8
// THe UTC time zone TZ should initially default to 0, and then be adjustable between -11:45 and +12
// there are some location such as Kiribati that are +14, this is ignored, but would need to be enabled if showing date (which current code does not)
// allow adjustments in 15 minute increments
// values adjusable by switch.
// values to be stored in eeprom
int TZ_HOUR;
int TZ_MINUTE;
int TZ_HOUR_addr = 3;
int TZ_MINUTE_addr = 4;



// buzzer inputs
// the intent is that these values are stored in eeprom
// AnalogWrite supplies a squarewave.
int BUZZER_VOLUME;  //0 (silent) to 255 (loudest). Adjustable by push button.
int BUZZER_DURATION; //Adjustable by button. Potential values are 1,2,4,8,16,32,64,128,256,512,1024.
int BUZZER_VOLUME_addr = 1;
int BUZZER_DURATION_addr = 2;

int max_brightness_setting;
int bright_addr = 6;

//12 hr format or 24 hr format
int type;
int type_addr = 5;

int GPS_age_MAX_pow;
int GPS_addr = 7;
long GPS_age_MAX;

// define the digital pin number for each button

int Button2 = 6;   // buzzer duration,  1d
int Button1 = 8;   // buzzer volume,    1U
int Button8 = 5;   // brightness,       1L
int Button7 = 9;   // gps age max       1R
int Button6 = 7;   // gps age,          1C
int Button3 = 11;  // TZ hr,            2L
int Button5 = 10;  // 12/24 hr,         2C
int Button4 = 12;  // TZ mins,          2R


void setup()
{
  Serial.begin(9600);   //for the serial monitor

  // define pins for buttons
  // Arduino Nano pin D5
  pinMode(5, INPUT);
  // Arduino Nano pin D6
  pinMode(6, INPUT);
  // Arduino Nano pin D7
  pinMode(7, INPUT);
  // Arduino Nano pin D8
  pinMode(8, INPUT);
  // Arduino Nano pin D9
  pinMode(9, INPUT);
  // Arduino Nano pin D10
  pinMode(10, INPUT);
  // Arduino Nano pin D11
  pinMode(11, INPUT);
  // Arduino Nano pin D12
  pinMode(12, INPUT);

  // prevent buzzer from buzzing unintentionally
  analogWrite (speakerPin, 0);
  pinMode (speakerPin, OUTPUT); //beeper
  // prevent buzzer from buzzing unintentionally
  analogWrite (speakerPin, 0);

  // get stored values from EEPROM

  // EEPROM.write(BUZZER_VOLUME_addr, BUZZER_VOLUME);
  // EEPROM.write(BUZZER_DURATION_addr, BUZZER_DURATION);
  BUZZER_VOLUME = EEPROM.read(BUZZER_VOLUME_addr);
  BUZZER_DURATION = EEPROM.read(BUZZER_DURATION_addr);

  // EEPROM.write(TZ_HOUR_addr, TZ_HOUR);
  // EEPROM.write(TZ_MINUTE_addr, TZ_MINUTE);
  TZ_HOUR = EEPROM.read(TZ_HOUR_addr);
  TZ_MINUTE = EEPROM.read(TZ_MINUTE_addr);

  //EEPROM.write(type_addr, type);
  type = EEPROM.read(type_addr); // this is 12 hr or 24 hr

  max_brightness_setting = EEPROM.read(bright_addr);

  GPS_age_MAX_pow = EEPROM.read(GPS_addr); // this is the max gps age before deciding signal is lost
  GPS_age_MAX = pow(2.0, GPS_age_MAX_pow / 2.0);

  // begin communication with s7s serial channel
  s7s.begin(9600);
  delay(20);
  // Clear the display, and then turn on all segments and decimals
  clearDisplay();  // Clears display, resets cursor
  s7s.print("-HI-");  // Displays -HI- using all digits
  setDecimals(0b111111);  // Turn on all decimals, colon, apostrophe

  // Flash brightness values at the beginning
  setBrightness(0);  // Lowest brightness
  delay(1500);
  setBrightness(127);  // Medium brightness
  delay(1500);
  setBrightness(255);  // High brightness
  delay(1500);

  // Clear the display before jumping into loop
  clearDisplay();

  myStats.clear(); //explicitly start clean for the stats
}


void loop()
{

  // if user presses button 1, adjust buzzer volume
  while (digitalRead(Button1) == 1) {
    BUZZER_VOLUME = BUZZER_VOLUME + 1;
    if (BUZZER_VOLUME > 8) {
      BUZZER_VOLUME = -2;
    }
    EEPROM.write(BUZZER_VOLUME_addr, BUZZER_VOLUME);
    analogWrite (speakerPin, pow(2.0, BUZZER_VOLUME));
    delay (pow(2.0, BUZZER_DURATION));
    analogWrite (speakerPin, 0);
    s7s.begin(9600);
    clearDisplay();
    setCursor(0);  // Sets cursor to far left
    int aaa = pow(2.0, BUZZER_VOLUME) + 0.0;
    sprintf(tempString, "%4d", aaa);
    s7s.print(tempString);
    smartDelay(500);
    Serial.print(" BUZZER_VOLUME:");
    Serial.print(aaa);
  }

  // if user presses button 2, adjust buzzer duration
  while (digitalRead(Button2) == 1) {
    BUZZER_DURATION = BUZZER_DURATION + 1;
    if (BUZZER_DURATION > 10) {
      BUZZER_DURATION = -1;
    }
    EEPROM.write(BUZZER_DURATION_addr, BUZZER_DURATION);
    analogWrite (speakerPin, pow(2.0, BUZZER_VOLUME));
    delay (pow(2.0, BUZZER_DURATION));
    analogWrite (speakerPin, 0);
    s7s.begin(9600);
    clearDisplay();
    setCursor(0);  // Sets cursor to far left
    int aaa = pow(2.0, BUZZER_DURATION) + 0.0;
    sprintf(tempString, "%4d", aaa);
    s7s.print(tempString);
    smartDelay(500);
    Serial.print(" BUZZER_DURATION:");
    Serial.print(aaa);
  }

  // if user presses button 3, adjust hour
  while (digitalRead(Button3) == 1) {
    TZ_HOUR = TZ_HOUR + 1;
    if (TZ_HOUR > 12) {
      TZ_HOUR = -11;
    }
    EEPROM.write(TZ_HOUR_addr, TZ_HOUR);
    analogWrite (speakerPin, pow(2, BUZZER_VOLUME));
    delay (pow(2, BUZZER_DURATION));
    analogWrite (speakerPin, 0);
    s7s.begin(9600);
    clearDisplay();
    setCursor(0);  // Sets cursor to far left
    sprintf(tempString, "%4d", TZ_HOUR);
    s7s.print(tempString);
    smartDelay(500);
    Serial.print(" TZ_HOUR:");
    Serial.print(TZ_HOUR);
  }

  // if user presses button 4, adjust minutes
  while (digitalRead(Button4) == 1) {
    TZ_MINUTE = TZ_MINUTE + 15;
    if (TZ_MINUTE > 45) {
      TZ_MINUTE = 0;
    }
    EEPROM.write(TZ_MINUTE_addr, TZ_MINUTE);
    analogWrite (speakerPin, pow(2, BUZZER_VOLUME));
    delay (pow(2, BUZZER_DURATION));
    analogWrite (speakerPin, 0);
    s7s.begin(9600);
    clearDisplay();
    setCursor(0);  // Sets cursor to far left
    sprintf(tempString, "%4d", TZ_MINUTE);
    s7s.print(tempString);
    smartDelay(500);
    Serial.print(" TZ_MINUTE:");
    Serial.print(TZ_MINUTE);
  }

  // if user presses button 5, adjust 12 hr / 24 hr time and beep and display
  while (digitalRead(Button5) == 1) {
    type = type + 12;
    if (type > 24) {
      type = 12;
    }
    EEPROM.write(type_addr, type);
    analogWrite (speakerPin, pow(2, BUZZER_VOLUME));
    delay (pow(2, BUZZER_DURATION));
    analogWrite (speakerPin, 0);
    s7s.begin(9600);
    clearDisplay();
    setCursor(0);  // Sets cursor to far left
    sprintf(tempString, "%4d", type);
    s7s.print(tempString);
    smartDelay(500);
    Serial.print(" type:");
    Serial.print(type);
  }

  // if user presses button 8, adjust display brightness
  while (digitalRead(Button8) == 1) {
    max_brightness_setting = max_brightness_setting + 10;
    if (max_brightness_setting > 255) {
      max_brightness_setting = 5;
    }
    EEPROM.write(bright_addr, max_brightness_setting);
    analogWrite (speakerPin, pow(2, BUZZER_VOLUME));
    delay (pow(2, BUZZER_DURATION));
    analogWrite (speakerPin, 0);
    s7s.begin(9600);
    clearDisplay();
    setBrightness(max_brightness_setting);
    setCursor(0);  // Sets cursor to far left
    sprintf(tempString, "%4d", max_brightness_setting);
    s7s.print(tempString);
    smartDelay(500);
    Serial.print(" max_brightness_setting:");
    Serial.print(max_brightness_setting);
  }

  // if user presses button 6, display gps age
  while (digitalRead(Button6) == 1) {
    long GPS_age = gps.time.age();
    GPS_age = min(9999 , GPS_age );
    s7s.begin(9600);
    clearDisplay();
    setCursor(0);  // Sets cursor to far left
    sprintf(tempString, "%4d", GPS_age);
    s7s.print(tempString);
    smartDelay(500);
    Serial.print(" GPS_age:");
    Serial.print(GPS_age);
  }

  // if user presses button 7, adjust GPS_age_MAX
  // GPS_age_MAX = EEPROM.read(GPS_addr);
  while (digitalRead(Button7) == 1) {
    GPS_age_MAX_pow++ ;
    if (GPS_age_MAX_pow > 28) {
      GPS_age_MAX_pow = 18;
    }
    GPS_age_MAX = pow(2.0, GPS_age_MAX_pow / 2.0);
    int aaa = min(9999 , GPS_age_MAX );
    EEPROM.write(GPS_addr, GPS_age_MAX_pow);
    analogWrite (speakerPin, pow(2, BUZZER_VOLUME));
    delay (pow(2, BUZZER_DURATION));
    analogWrite (speakerPin, 0);
    s7s.begin(9600);
    clearDisplay();
    setCursor(0);  // Sets cursor to far left
    sprintf(tempString, "%4d", aaa);
    s7s.print(tempString);
    smartDelay(500);
    Serial.print(" GPS_age_MAX_pow:");
    Serial.print(GPS_age_MAX_pow);
    Serial.print(" GPS_age_MAX:");
    Serial.print(GPS_age_MAX);
  }

  // begin communication with gps serial channel
  ss.begin(GPSBaud);    //gps communication
  smartDelay(150);     //include a delay for gps data to flow in
  // receive the data from the gps module
  int GPS_YEAR = gps.date.year();
  int GPS_HOUR = gps.time.hour();
  int GPS_MINUTE = gps.time.minute();
  int GPS_SECOND = gps.time.second();
  long GPS_age = gps.time.age();


  // as of 2016, gps time is 18 seconds ahead of UTC time.
  // the gps receiver used in this project accounts for this and outputs UTC time

  // there are 86400 seconds in a day; 60 * 60 * 24
  // GPS_SECS is the number of seconds since midnight, UTC
  long GPS_SECS = GPS_HOUR * 60 * 60L + GPS_MINUTE * 60 + GPS_SECOND; // L is added to ensure long variable is used
  // local secs is the number of seconds since midnight, local time
  long LOCAL_SECS = GPS_SECS + TZ_HOUR * 60 * 60 + TZ_MINUTE * 60 + 0L;
  // adjust LOCAL_SECS incase out of range 0 to 86400
  if (LOCAL_SECS < 0) {
    LOCAL_SECS = LOCAL_SECS + 86400;
  }
  if (LOCAL_SECS > 86400) {
    LOCAL_SECS = LOCAL_SECS - 86400;
  }
  // calculate local time
  long LOCAL_HOUR = LOCAL_SECS / 60 / 60L;
  long LOCAL_MINUTE = (LOCAL_SECS - LOCAL_HOUR * 60 * 60L) / 60L;
  long LOCAL_SECOND = LOCAL_SECS - LOCAL_HOUR * 60 * 60 - LOCAL_MINUTE * 60L;

  // if type is 12 hr clock then adjust
  if (type == 12 & LOCAL_HOUR >= 13) {
    LOCAL_HOUR = LOCAL_HOUR - 12;
  }


  long myStats_count;
  long myStats_average;
  float myStats_pop_stdev;
  if (power_on_lock == false) {
    secos = millis() / 1000;
    power_on = LOCAL_SECS - secos;
    while (power_on < 0) {
      power_on = power_on + 86400;
    }
    myStats.add(power_on);
    myStats_count = myStats.count();
    myStats_average = myStats.average();
    myStats_pop_stdev = myStats.pop_stdev();
    if (myStats_count == numreadings & myStats_pop_stdev <= sd_threshold) {
      power_on_lock = true;
      power_on_final = myStats_average;
      analogWrite (speakerPin, pow(2.0, BUZZER_VOLUME));
      delay (pow(2.0, BUZZER_DURATION));
      analogWrite (speakerPin, 0);
      smartDelay(250);
      analogWrite (speakerPin, pow(2.0, BUZZER_VOLUME));
      delay (pow(2.0, BUZZER_DURATION));
      analogWrite (speakerPin, 0);
    }
    if (myStats_count >= numreadings) {
      myStats.clear();
    }
  }
  else {
    secos = millis() / 1000;
    nano_time = power_on_final + secos;
    while (nano_time > 86400) {
      nano_time = nano_time - 86400;
    }
    if (abs(nano_time - LOCAL_SECS) < 60) {
      // check if the nano thinks the time is valid
      nano_time_valid = true;
    }
    else {
      nano_time_valid = false;
    }
  }
  // if millis overflows, set power_on_lock = false
  if (millis() > (4294967295 - 10000)) {
    power_on_lock = false;
  }

  // after 1 day reset power_on_lock = false 
  // this is because the nano does not have an accurate timer and will drift after a few days
  if (nano_time == power_on_final){
    power_on_lock = false;
    power_on_final = 0;
    nano_time = 0;
  }

  

  // calculate local time using nano_time
  long NANO_HOUR = nano_time / 60 / 60L;
  long NANO_MINUTE = (nano_time - NANO_HOUR * 60 * 60L) / 60L;
  // if type is 12 hr clock then adjust
  if (type == 12 & NANO_HOUR >= 13) {
    NANO_HOUR = NANO_HOUR - 12;
  } 

  // if gps signal is lost and nano_time_valid=false then use nano_time as the actual time
  if (power_on_lock == true & nano_time_valid == false) {
    LOCAL_MINUTE = NANO_MINUTE;
    LOCAL_HOUR = NANO_HOUR;
  }


  // beep on the hour
  if (true) {
    if (LOCAL_MINUTE == 0 & GPS_YEAR > 2017 & GPS_age > 0 & GPS_age <= GPS_age_MAX & validity_score < 30000 & power_on_lock) { //2 minute buffer, in ms
      if ((millis() - BEEP_TIMER) > 2 * 1000 * 60L) {
        analogWrite (speakerPin, pow(2, BUZZER_VOLUME));
        delay (pow(2, BUZZER_DURATION));
        analogWrite (speakerPin, 0);
        BEEP_TIMER = millis();
      }
      // Serial.print(" MILLIS():");
      // Serial.print(millis());
      // Serial.print(" BEEP_TIMER:");
      // Serial.print(BEEP_TIMER);
    }
  }

  // the below doesnt make sense, try not ending serial to see if its even necessary to end it
  // end communication with gps serial channel
  // ss.begin(GPSBaud);    //gps communication

  // begin communication with s7s serial channel
  s7s.begin(9600);

  // determine counter value to send time to display
  if (LOCAL_HOUR == 0){
    counter = 12 * 100 + LOCAL_MINUTE;
  }
  else{
    counter = LOCAL_HOUR * 100 + LOCAL_MINUTE;
  }


  if ((GPS_YEAR < 2018 or GPS_age < 0 or GPS_age > GPS_age_MAX) and power_on_lock == false) {
    clearDisplay();  // Clears display, resets cursor
    // clearDisplay();  // Clears display, resets cursor
    s7s.begin(9600);
    s7s.print("gpS ");  // Displays "GPS ".
    for (int k = 2; k <= 10; k = k + 1) {
      s7s.begin(9600);
      setBrightness(max_brightness_setting / 10 * k);
      s7s.print("gpS ");  // Displays "GPS ".
      smartDelay(120);
    }
    for (int k = 9; k > 1; k = k - 1) {
      s7s.begin(9600);
      setBrightness(max_brightness_setting / 10 * k);
      s7s.print("gpS ");  // Displays "GPS ".
      smartDelay(120);
    }
  }

  else {
    // Set cursor position
    setCursor(0);  // Sets cursor to far left
    setBrightness(max_brightness_setting);

    // Magical sprintf creates a string for us to send to the s7s.
    //  The %4d option creates a 4-digit integer.
    sprintf(tempString, "%4d", counter);

    // This will output the tempString to the S7S
    s7s.print(tempString);
    setDecimals(0b00010000);  // Sets colon on
    delay(20);
  }



  // check validity of time from gps by seeing if gps
  // second is incrementing by a similar amount to millis()
  nowmillis = millis();
  if (GPS_SECOND != LAST_GPS_SECOND) {
    LAST_GPS_SECOND = GPS_SECOND;
    lastmillis = nowmillis;
    validity_score = nowmillis - lastmillis;
  }
  validity_score = nowmillis - lastmillis;

  // print all the things
  if (false) {
    Serial.print(" :");
    Serial.print(GPS_YEAR);
    Serial.print(" :");
    Serial.print(GPS_HOUR);
    Serial.print(" :");
    Serial.print(GPS_MINUTE);
    Serial.print(" :");
    Serial.print(GPS_SECOND);
    Serial.print(" GPS_SECS:");
    Serial.print(GPS_SECS);
    Serial.print(" LOCAL_SECS:");
    Serial.print(LOCAL_SECS);
    Serial.print(" LOCAL_HOUR:");
    Serial.print(LOCAL_HOUR);
    Serial.print(" LOCAL_MINUTE:");
    Serial.print(LOCAL_MINUTE);
    Serial.print(" LOCAL_SECOND:");
    Serial.print(LOCAL_SECOND);
    Serial.print(" secos:");
    Serial.print(secos);
    Serial.print(" power_on:");
    Serial.print(power_on);
    Serial.print(" Count:");
    Serial.print(myStats_count);
    Serial.print(" Average:");
    Serial.print(myStats_average);
    Serial.print(" Stdev:");
    Serial.print(myStats_pop_stdev, 3);
    Serial.print(" power_on_lock:");
    Serial.print(power_on_lock);
    Serial.print(" power_on_final:");
    Serial.print(power_on_final);
    Serial.print(" nano_time:");
    Serial.print(nano_time);
    Serial.print(" nano_time_valid:");
    Serial.print(nano_time_valid);
    Serial.print(" counter:");
    Serial.print(counter);
    Serial.print(" GPS_age:");
    Serial.print(GPS_age);
    Serial.print(" validity_score:");
    Serial.print(validity_score);
    Serial.println();
  }

  int r = nano_time - LOCAL_SECS;
  //  Serial.println(r, DEC);
  //  Serial.println(nano_time, DEC);

}  // end of void loop



// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  ss.begin(GPSBaud);
  do
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}




// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplay()
{
  s7s.write(0x76);  // Clear display command
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightness(byte value)
{
  s7s.write(0x7A);  // Set brightness command byte
  s7s.write(value);  // brightness data byte
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimals(byte decimals)
{
  s7s.write(0x77);
  s7s.write(decimals);
}

// Set the cursor position
//  0 = left most, 3 = right most.
void setCursor(byte cursor_pos)
{
  s7s.write(0x79);
  s7s.write(cursor_pos);
}




// can potentially delete this section
static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    Serial.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    Serial.print(sz);
  }

  if (!t.isValid())
  {
    Serial.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    Serial.print(sz);
  }

  smartDelay(0);
}


