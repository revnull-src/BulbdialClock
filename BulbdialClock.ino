/*
 BulbdialClock.ino

 Default software for the Bulbdial Clock kit designed by
 Evil Mad Scientist Laboratories: http://www.evilmadscientist.com/go/bulbdialkit

 "Trying" to work with Arduino 1.0 by Ray Ramirez
 Also requires Time library:  http://www.arduino.cc/playground/Code/Time

 Target: ATmega168, clock at 16 MHz.

 Version 1.0.1 - 1/14/2012
 Copyright (c) 2009 Windell H. Oskay.  All right reserved.
 http://www.evilmadscientist.com/

 This library is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <EEPROM.h>            // For saving settings
#include <Wire.h>              // For optional RTC module
#include <Time.h>              // For optional Serial Sync

/*
 EEPROM variables that are saved:  7

 * Brightness setting (range: 1-8)  Default: 8   (Fully bright)

 * Red brightness  (range: 0-63)    Default: 20
 * Green brightness (range: 0-63)   Default: 63
 * Blue brightness (range: 0-63)    Default: 63

 * Time direction (Range: 0,1)      Default: 0  (Clockwise)
 * Fade style (Range: 0,1)         Default: 1  (Fade enabled)

 * Alignment mode                  Default: 0

*/

// "Factory" default configuration can be configured here:
#define MainBrightDefault 8

#define RedBrightDefault 63  // Use 63, default, for kits with monochrome LEDs!
#define GreenBrightDefault 63
#define BlueBrightDefault 63
#define FadeModeDefault 1
#define CCWDefault 0
#define AlignModeDefault 0

#define TIME_MSG_LEN 11  // time sync to PC is HEADER followed by unix time_t as ten ascii digits
#define TIME_HEADER 255  // Header tag for serial time sync message

// The buttons are located at D5, D6, & D7.
#define buttonmask 224

// LED outputs B0-B2:
#define LEDsB 7

// C0-C3 are LED outputs:
#define LEDsC 15

// TX, PD2,PD3,PD4 are LED outputs.
#define LEDsD 28

// Negative masks of those LED positions, for quick turn-off:
#define LEDsBInv 248
#define LEDsCInv 240
#define LEDsDInv 227

#define LED_B_Off();   DDRB &= LEDsBInv;  PORTB &= LEDsBInv;
#define LED_C_Off();   DDRC &= LEDsCInv;  PORTC &= LEDsCInv;
#define LED_D_Off();   DDRD &= LEDsDInv;  PORTD &= LEDsDInv;

#define AllLEDsOff();  LED_B_Off(); LED_C_Off(); LED_D_Off();

#define tempfade 63


void TakeHigh(byte LEDline)
{
  switch( LEDline )
  {
  case 1:
    DDRB  |= 4;
    PORTB |= 4;
    break;
  case 2:
    DDRC  |= 1;
    PORTC |= 1;
    break;
  case 3:
    DDRC  |= 2;
    PORTC |= 2;
    break;
  case 4:
    DDRC  |= 4;
    PORTC |= 4;
    break;
  case 5:
    DDRC  |= 8;
    PORTC |= 8;
    break;
  case 6:
    DDRD  |= 16;
    PORTD |= 16;
    break;
  case 7:
    DDRD  |= 4;
    PORTD |= 4;
    break;
  case 8:
    DDRB  |= 1;
    PORTB |= 1;
    break;
  case 9:
    DDRD  |= 8;
    PORTD |= 8;
    break;
  case 10:
    DDRB  |= 2;
    PORTB |= 2;
    break;
    // default:
  }
}


void TakeLow(byte LEDline)
{
  switch( LEDline )
  {
  case 1:
    DDRB  |= 4;
    PORTB &= 251;
    break;
  case 2:
    DDRC  |= 1;
    PORTC &= 254;
    break;
  case 3:
    DDRC  |= 2;
    PORTC &= 253;
    break;
  case 4:
    DDRC  |= 4;
    PORTC &= 251;
    break;
  case 5:
    DDRC  |= 8;
    PORTC &= 247;
    break;
  case 6:
    DDRD  |= 16;
    PORTD &= 239;
    break;
  case 7:
    DDRD  |= 4;
    PORTD &= 251;
    break;
  case 8:
    DDRB  |= 1;
    PORTB &= 254;
    break;
  case 9:
    DDRD  |= 8;
    PORTD &= 247;
    break;
  case 10:
    DDRB  |= 2;
    PORTB &= 253;
    break;
    // default:
  }
}


void delayTime(byte time)
{
  unsigned int delayvar;
  delayvar = 0;
  while (delayvar <=  time)
  {
    asm("nop");
    delayvar++;
  }
}


boolean getPCtime() {
  // if time sync available from serial port, update time and return true
  while(Serial.available() >=  TIME_MSG_LEN ){  // time message consists of a header and ten ascii digits
    if( Serial.read() == TIME_HEADER ) {
      time_t pctime = 0;
      for(int i=0; i < TIME_MSG_LEN -1; i++){
        char c= Serial.read();
        if( c >= '0' && c <= '9'){
          pctime = (10 * pctime) + (c - '0') ;  // convert digits to a number
        }
      }
      // DateTime.sync(pctime);  // Sync Arduino clock to the time received on the serial port
      setTime(pctime);
      return true;  // return true if time message received on the serial port
    }
  }
  return false;  // if no message return false
}


void printDigits(byte digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits,DEC);
}


void digitalClockDisplay(){
  // digital clock display of current date and time
  // Serial.print(DateTime.Hour,DEC);
  // printDigits(DateTime.Minute);
  // printDigits(DateTime.Second);
  Serial.print(hour(),DEC);
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  // Serial.print(DateTimeStrings.dayStr(DateTime.DayofWeek));
  Serial.print(dayStr(weekday()));
  Serial.print(" ");
  // Serial.print(DateTimeStrings.monthStr(DateTime.Month));
  Serial.print(monthStr(month()));
  Serial.print(" ");
  // Serial.println(DateTime.Day,DEC);
  Serial.println(day(),DEC);
}

const byte SecHi[30] = {
  2,3,4,5,6,1,3,4,5,6,1,2,4,5,6,1,2,3,5,6,1,2,3,4,6,1,2,3,4,5};
const byte SecLo[30] = {
  1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,4,5,5,5,5,5,6,6,6,6,6};

const byte MinHi[30] = {
  1,7,1,8,1,9,2,7,2,8,2,9,3,7,3,8,3,9,4,7,4,8,4,9,5,7,5,8,5,9};
const byte MinLo[30] = {
  7,1,8,1,9,1,7,2,8,2,9,2,7,3,8,3,9,3,7,4,8,4,9,4,7,5,8,5,9,5};

const byte HrHi[12]  = {
  10, 1, 2,10,10, 6, 3, 10,10, 4, 5,10};
const byte HrLo[12]  = {
  1,10,10, 2, 6,10,10, 3, 4,10,10, 5};

byte SecNow;
byte MinNow;
byte HrNow;
byte HrDisp,MinDisp, SecDisp;

#define EELength 7
byte EEvalues[EELength];

// Variables to store brightness of the three LED rings.
byte HourBright;
byte MinBright;
byte SecBright;
byte MainBright;

unsigned long LastTime;
unsigned long TimeNow;
byte TimeSinceButton;
byte LastSavedBrightness;

byte PINDLast;

// Modes:
byte CCW;
byte ExtRTC;
byte SleepMode;
byte FadeMode;

byte VCRmode;  // In VCR mode, the clock blinks at you because the time hasn't been set yet.
byte FactoryResetDisable;  // To make sure that we don't accidentally reset the settings...

byte SettingTime;
byte AlignMode;
byte OptionMode;
byte AlignValue;
char AlignRate;

byte AlignLoopCount;
byte StartingOption;

byte HoldTimeSet;
byte HoldOption;
byte HoldAlign;

byte MomentaryOverridePlus;
byte MomentaryOverrideMinus;
byte MomentaryOverrideZ;

unsigned long  prevtime;
unsigned long millisCopy;

byte SecNext,  MinNext, HrNext;
byte h0, h1, h2, h3, h4, h5;
byte l0, l1, l2, l3, l4, l5;
byte d0, d1, d2, d3, d4, d5;
byte HrFade1, HrFade2, MinFade1, MinFade2, SecFade1, SecFade2;


void ApplyDefaults (void) {
  /*
   * Brightness setting (range: 1-8)  Default: 8  (Fully bright)
   * Red brightness  (range: 0-63)    Default: 20
   * Green brightness (range: 0-63)   Default: 63
   * Blue brightness (range: 0-63)    Default: 63
   * Time direction (Range: 0,1)      Default: 0  (Clockwise)
   * Fade style (Range: 0,1)          Default: 1  (Fade enabled)
  */

  MainBright = MainBrightDefault;
  HourBright = RedBrightDefault;
  MinBright = GreenBrightDefault;
  SecBright = BlueBrightDefault;
  CCW = CCWDefault;
  FadeMode = FadeModeDefault;

}


void EEReadSettings (void) {  // TODO: Detect ANY bad values, not just 255.

  byte detectBad = 0;
  byte value = 255;

  value = EEPROM.read(0);

  if (value > 8)
    detectBad = 1;
  else
    MainBright = value;  // MainBright has maximum possible value of 8.

  if (value == 0)
    MainBright = 1;  // Turn back on when power goes back on-- don't leave it dark.

  value = EEPROM.read(1);
  if (value > 63)
    detectBad = 1;
  else
    HourBright = value;

  value = EEPROM.read(2);
  if (value > 63)
    detectBad = 1;
  else
    MinBright = value;

  value = EEPROM.read(3);
  if (value > 63)
    detectBad = 1;
  else
    SecBright = value;

  value = EEPROM.read(4);
  if (value > 1)
    detectBad = 1;
  else
    CCW = value;

  value = EEPROM.read(5);
  if (value == 255)
    detectBad = 1;
  else
    FadeMode = value;

  if (detectBad)
    ApplyDefaults();

  LastSavedBrightness = MainBright;

}

void EESaveSettings (void){
  //EEPROM.write(Addr, Value);

  // Careful if you use  this function: EEPROM has a limited number of write
  // cycles in its life.  Good for human-operated buttons, bad for automation.

  EEPROM.write(0, MainBright);
  EEPROM.write(1, HourBright);
  EEPROM.write(2, MinBright);
  EEPROM.write(3, SecBright);
  EEPROM.write(4, CCW);
  EEPROM.write(5, FadeMode);

  LastSavedBrightness = MainBright;

  // Optional: Blink LEDs off to indicate when we're writing to the EEPROM
  // AllLEDsOff();
  // delay(100);

}


void normalTimeDisplay(void) {

  SecDisp = (SecNow + 30);  // Offset by 30 s to project *shadow* in the right place.
  if ( SecDisp > 59)
    SecDisp -= 60;
  SecDisp >>= 1;  // Divide by two, since there are 30 LEDs, not 60.

  SecNext = SecDisp + 1;
  if (SecNext > 29)
    SecNext = 0;

  MinDisp = (MinNow + 30);  // Offset by 30 m to project *shadow* in the right place.
  if ( MinDisp > 59)
    MinDisp -= 60;
  MinDisp >>= 1;  // Divide by two, since there are 30 LEDs, not 60.

  MinNext = MinDisp + 1;
  if (MinNext > 29)
    MinNext = 0;

  HrDisp = (HrNow + 6);  // Offset by 6 h to project *shadow* in the right place.

  if ( HrDisp > 11)
    HrDisp -= 12;

  HrNext = HrDisp + 1;
  if (HrNext > 11)
    HrNext = 0;

}


void normalFades(void) {

  if (FadeMode){
    // Normal time display
    if (SecNow & 1)  // ODD time
    {
      SecFade2 = (63*((millisCopy - LastTime))/1000);
      SecFade1 = 63 - SecFade2;
    }

    if (MinNow & 1)  // ODD time
    {
      if (SecNow == 59){
        MinFade2 = SecFade2;
        MinFade1 = SecFade1;
      }
    }

    if (MinNow == 59)  // End of the hour, only:
    {
      if (SecNow == 59){
        HrFade2 = SecFade2;
        HrFade1 = SecFade1;
      }
    }
  }
  else {  // no fading

    HrFade1 = tempfade;
    MinFade1 = tempfade;
    SecFade1 = tempfade;
  }
}


void RTCsetTime(byte hourIn, byte minuteIn, byte secondIn)
{
  Wire.beginTransmission(104);  // 104 is DS3231 device address
  Wire.write("0");  // start at register 0

  byte ts = secondIn / 10;
  byte os = secondIn - ts*10;
  byte ss = (ts << 4) + os;

  Wire.write(ss);  //Send seconds as BCD

  byte tm = minuteIn /10;
  byte om = minuteIn - tm*10;
  byte sm = (tm << 4 ) | om;

  Wire.write(sm);  //Send minutes as BCD

  byte th = hourIn /10;
  byte oh = hourIn - th*10;
  byte sh = (th << 4 ) | oh;

  Wire.write(sh);  //Send hours as BCD

  Wire.endTransmission();

}

byte RTCgetTime()
{ // Read out time from RTC module, if present
  // send request to receive data starting at register 0

  byte status = 0;
  Wire.beginTransmission(104);  // 104 is DS3231 device address
  Wire.write("0");  // start at register 0
  Wire.endTransmission();
  Wire.requestFrom(104, 3);  // request three bytes (seconds, minutes, hours)

  int seconds, minutes, hours;
  unsigned int temptime1, temptime2;
  byte updatetime = 0;

  while(Wire.available())
  {
    status = 1;
    seconds = Wire.read();  // get seconds
    minutes = Wire.read();  // get minutes
    hours = Wire.read();    // get hours
  }

  // IF time is off by MORE than two seconds, then correct the displayed time.
  // Otherwise, DO NOT update the time, it may be a sampling error rather than an
  // actual offset.
  // Skip checking if minutes == 0. -- the 12:00:00 rollover is distracting,
  // UNLESS this is the first time running after reset.

  // if (ExtRTC) is equivalent to saying,  "if this has run before"

  if (status){
    seconds = (((seconds & 0b11110000)>>4)*10 + (seconds & 0b00001111));  // convert BCD to decimal
    minutes = (((minutes & 0b11110000)>>4)*10 + (minutes & 0b00001111));  // convert BCD to decimal
    hours = (((hours & 0b00110000)>>4)*10 + (hours & 0b00001111));  // convert BCD to decimal (assume 24 hour mode)

    //Optional: report time::
    // Serial.print(hours); Serial.print(":"); Serial.print(minutes); Serial.print(":"); Serial.println(seconds);

    if ((minutes) && (MinNow) ){
      temptime1 = 3600*hours + 60*minutes + seconds;  // Values read from RTC
      temptime2 = 3600*HrNow + 60*MinNow + SecNow;    // Internally stored time estimate.

      if (temptime1 > temptime2)
      {
        if ((temptime1 - temptime2) > 2)
          updatetime = 1;
      }
      else
      {
        if ((temptime2 - temptime1) > 2)
          updatetime = 1;
      }
    }

    if (ExtRTC == 0)
      updatetime = 1;

    if (updatetime)
    {
      SecNow = seconds;
      MinNow = minutes;
      HrNow = hours;

      if ( HrNow > 11)  // Convert 24-hour mode to 12-hour mode
        HrNow -= 12;
    }
  }

  return status;
}


void setup()  // run once, when the sketch starts
{
  Serial.begin(19200);
  // DateTime.sync(0);
  setTime(0);

  PORTB = 0;
  PORTC = 0;
  PORTD = 0;

  DDRB = 0;  // All inputs
  DDRC = 0;  // All inputs
  DDRD = _BV(1);  //All inputs except TX.

  PORTD = buttonmask;  // Pull-up resistors for buttons

  SecNow = 0;
  HrNow = 0;
  MinNow = 0;
  TimeNow = millis();
  SecDisp = 0;
  MinDisp = 0;

  EEReadSettings();

  /*
   MainBright = 8;  // 8 is maximum value.
   HourBright = 30;
   MinBright = 63;
   SecBright = 63;
   CCW = 0;  // presume clockwise, not counterclockwise.
   FadeMode = 1;  // Presume fading is enabled.
  */

  VCRmode = 1;  // Time is NOT yet set.
  FactoryResetDisable = 0;
  TimeSinceButton = 0;

  PINDLast =  PIND & buttonmask;
  // ButtonHold = 0;

  HoldTimeSet = 0;
  HoldOption = 0;
  HoldAlign = 0;
  MomentaryOverridePlus = 0;
  MomentaryOverrideMinus = 0;
  MomentaryOverrideZ = 0;

  SleepMode = 0;

  SettingTime = 0;  // Normally 0.
  // 1: hours, 2: minutes, 3: seconds, 4: not setting time

  AlignMode  = 0;  // Normally 0.
  OptionMode  = 0;  // Normally 0.
  AlignValue = 0;
  AlignRate = 2;
  AlignLoopCount = 0;
  StartingOption = 0;

  Wire.begin();

  /*
   // HIGHLY OPTIONAL: Set jardcoded RTC Time from within the program.
   // Example: Set time to 2:52:45.

   RTCsetTime(2,52,45);
  */

  ExtRTC = 0;

  // Check if RTC is available, and use it to set the time if so.
  ExtRTC = RTCgetTime();
  // If no RTC is found, no attempt will be made to use it thereafter.

  if (ExtRTC)  // If time is already set from the RTC...
    VCRmode = 0;

}  // End Setup


void IncrAlignVal (void)
{
  AlignValue++;

  if (AlignMode < 5)  // seconds or minutes
  {
    if (AlignValue > 29)
      AlignValue = 0;
  }
  else
  {
    if (AlignValue > 11)
      AlignValue = 0;
  }
}


void DecrAlignVal (void)
{
  if (AlignValue > 0)
    AlignValue--;
  else if (AlignMode < 5)  // seconds or minutes
  {
    AlignValue = 29;
  }
  else  // hours
  {
    AlignValue = 11;
  }
}


void loop()
{
  byte HighLine, LowLine;
  byte PINDcopy;
  byte RefreshTime;

  RefreshTime = AlignMode + SettingTime + OptionMode;

  PINDcopy = PIND & buttonmask;

  if (PINDcopy != PINDLast)  // Button change detected
  {

    VCRmode = 0;  // End once any buttons have been pressed...
    TimeSinceButton = 0;

    if ((PINDcopy & 32) && ((PINDLast & 32) == 0))
    {  //"+" Button was pressed previously, and was just released!

      if ( MomentaryOverridePlus)
      {
        MomentaryOverridePlus = 0;
        // Ignore this transition if it was part of a hold sequence.
      }
      else
      {
        if (SleepMode)
          SleepMode = 0;
        else{
          if (AlignMode) {

            if ( AlignMode & 1)  // Odd mode:
            {
              if (AlignRate < 2)
                AlignRate++;
            }
            else
              IncrAlignVal();  //even mode

          }
          else if (OptionMode) {

            if (OptionMode == 1)
            {
              if (HourBright < 62)
                HourBright += 2;
            }
            if (OptionMode == 2)
            {
              if (MinBright < 62)
                MinBright += 2;
            }
            if (OptionMode == 3)
            {
              if (SecBright < 62)
                SecBright += 2;
            }
            if (OptionMode == 4)
            {
              CCW = 0;
            }
            if (OptionMode == 5)
            {
              FadeMode = 1;
            }

          }
          else if (SettingTime) {

            if (SettingTime == 1)
            {
              HrNow++;
              if (HrNow > 11)
                HrNow = 0;
            }
            if (SettingTime == 2)
            {
              MinNow++;
              if (MinNow > 59)
                MinNow = 0;
            }
            if (SettingTime == 3)
            {
              SecNow++;
              if (SecNow > 59)
                SecNow = 0;
            }
          }
          else {
            // Brightness control mode
            MainBright++;
            if (MainBright > 8)
              MainBright = 1;
          }
        }
      }
    }

    if ((PINDcopy & 64) && ((PINDLast & 64) == 0))
    {  //"-" Button was pressed and just released!

    VCRmode = 0;  // End once any buttons have been pressed...
    TimeSinceButton = 0;

      if ( MomentaryOverrideMinus)
      {
        MomentaryOverrideMinus = 0;
        // Ignore this transition if it was part of a hold sequence.
      }
      else
      {
        if (SleepMode)
          SleepMode = 0;
        else{
          if (AlignMode) {

            if ( AlignMode & 1)  // Odd mode:
            {
              if (AlignRate > -3)
                AlignRate--;
            }
            else
              DecrAlignVal();  //even mode

          }
          else if (OptionMode) {

            if (OptionMode == 1)
            {
              if (HourBright > 1)
                HourBright -= 2;
            }
            if (OptionMode == 2)
            {
              if (MinBright > 1)
                MinBright -= 2;
            }
            if (OptionMode == 3)
            {
              if (SecBright > 1)
                SecBright -= 2;
            }
            if (OptionMode == 4)
            {
              CCW = 1;
            }
            if (OptionMode == 5)
            {
              FadeMode = 0;
            }

          }
          else if (SettingTime) {
            if (SettingTime == 1)
            {
              if (HrNow > 0)
                HrNow--;
              else
                HrNow = 11;
            }
            if (SettingTime == 2)
            {
              if (MinNow > 0)
                MinNow--;
              else
                MinNow = 59;
            }
            if (SettingTime == 3)
            {
              if (SecNow > 0)
                SecNow--;
              else
                SecNow = 59;
            }
          }
          else {  //Normal brightness adjustment mode
            if (MainBright > 1)
              MainBright--;
            else
              MainBright = 8;
          }
        }
      }
    }

    if ((PINDcopy & 128) && ((PINDLast & 128) == 0))
    {  //"Z" Button was pressed and just released!

    VCRmode = 0;  // End once any buttons have been pressed...
    TimeSinceButton = 0;

      if ( MomentaryOverrideZ)
      {
        MomentaryOverrideZ = 0;
        // Ignore this transition if it was part of a hold sequence.
      }
      else
      {
        if (AlignMode) {

          AlignMode++;
          if (AlignMode > 6)
            AlignMode = 1;

          AlignValue = 0;
          AlignRate = 2;
        }
        else if (OptionMode) {

          OptionMode++;
          StartingOption = 0;

          if (OptionMode > 5)
            OptionMode = 1;
        }
        else if (SettingTime) {
          SettingTime++;
          if (SettingTime > 3)
            SettingTime = 1;
        }
        else {

          if (SleepMode == 0)
            SleepMode = 1;
          else
            SleepMode = 0;
        }
      }
    }
  }

  PINDLast = PINDcopy;
  millisCopy = millis();

  // The next if statement detects and deals with the millis() rollover.
  // This introduces an error of up to  1 s, about every 50 days.
  //
  // (If you have the standard quartz timebase, this will not dominate the inaccuracy.
  // If you have the optional RTC, this error will be corrected next time we read the
  // time from the RTC.)

  if (millisCopy < LastTime)
    LastTime = 0;

  if ((millisCopy - LastTime) >= 1000)
  {
    LastTime += 1000;

    // Check to see if any buttons are being held down:

    if (( PIND & buttonmask) == buttonmask)
    {
      // No buttons are pressed.
      // Reset the variables that check to see if buttons are being held down.

      HoldTimeSet = 0;
      HoldOption = 0;
      HoldAlign = 0;
      FactoryResetDisable = 1;

      if (TimeSinceButton < 250)
         TimeSinceButton++;

      if (TimeSinceButton == 10)  // 10 s after last button released...
      {
        if (LastSavedBrightness != MainBright)
        {
          EESaveSettings();
        }
      }
    }
    else
    {

      // Note which buttons are being held down

      if (( PIND & buttonmask) == 128)  // "+" and "-" are pressed down. "Z" is up.
      {
        HoldAlign++;  // We are holding for alignment mode.
        HoldOption = 0;
        HoldTimeSet = 0;
      }
      if (( PIND & buttonmask) == 64)  // "+" and "Z" are pressed down. "-" is up.
      {
        HoldOption++;  // We are holding for option setting mode.
        HoldTimeSet = 0;
        HoldAlign = 0;
      }
      if (( PIND & buttonmask) == 96)  // "Z" is pressed down. "+" and "-" are up.
      {
        HoldTimeSet++;  // We are holding for time setting mode.
        HoldOption = 0;
        HoldAlign = 0;
      }
    }

    if (HoldAlign == 3)
    {
      MomentaryOverridePlus = 1;   // Override momentary-action of switches
      MomentaryOverrideMinus = 1;  // since we've detected a hold-down condition.

      OptionMode = 0;
      SettingTime = 0;

      // Hold + and - for 3 s AT POWER ON to restore factory settings.
      if ( FactoryResetDisable == 0){
      ApplyDefaults();
      EESaveSettings();
      AllLEDsOff();  // Blink LEDs off to indicate restoring data
      delay(100);
      }
      else
      {
        if (AlignMode) {
          AlignMode = 0;
        }
        else {
          AlignMode = 1;
          AlignValue = 0;
          AlignRate = 2;
        }
      }
    }


    if (HoldOption == 3)
    {
      MomentaryOverridePlus = 1;
      MomentaryOverrideZ = 1;
      AlignMode = 0;
      SettingTime = 0;

      if (OptionMode) {
        OptionMode = 0;
        EESaveSettings();  // Save options if exiting option mode!
        AllLEDsOff();      // Blink LEDs off to indicate saving data
        delay(100);
      }
      else {
        OptionMode = 1;
        StartingOption = 0;
      }
    }

    if (HoldTimeSet == 3)
    {
      MomentaryOverrideZ = 1;

      if (AlignMode + OptionMode + SettingTime)  {
        // If we were in any of these modes, let's now return us to normalcy.
        // IF we are exiting time-setting mode, save the time to the RTC, if present:
        if (SettingTime && ExtRTC) {
          RTCsetTime(HrNow,MinNow,SecNow);
          AllLEDsOff();  // Blink LEDs off to indicate saving time
          delay(100);
        }

        if (OptionMode) {
          EESaveSettings();  // Save options if exiting option mode!
          AllLEDsOff();    // Blink LEDs off to indicate saving data
          delay(100);
        }

        SettingTime = 0;
      }
      else
      {  // Go to setting mode IF and ONLY IF we were in regular-clock-display mode.
        SettingTime = 1;  // Start with HOURS in setting mode.
      }

      AlignMode = 0;
      OptionMode = 0;

    }

    // Note: this section could act funny if you hold the buttons for 256 or more seconds.
    // So... um... don't do that.  :P

    SecNow++;

    if (SecNow > 59){
      SecNow = 0;
      MinNow++;

    if ((SettingTime == 0) && ExtRTC)  // Check value at RTC ONCE PER MINUTE, if enabled.
      RTCgetTime();                    // Do not check RTC time, if we are in time-setting mode.
    }

    if (MinNow > 59){
      MinNow = 0;
      HrNow++;

      if  (HrNow > 11)
        HrNow = 0;
    }

    RefreshTime = 1;

  }
  if (RefreshTime) {
    // Calculate which LEDs to light up to give
    // the correct shadows:

    if (AlignMode){

      if (AlignMode & 1) {  //ODD mode, auto-advances

        byte AlignRateAbs;  // Absolute value of AlignRate

        if (AlignRate >= 0){
          AlignRateAbs = AlignRate + 1;
        }
        else
          AlignRateAbs = -AlignRate;

        // Serial.println(AlignRateAbs,DEC);

        AlignLoopCount++;

        byte ScaleRate;
        if (AlignRateAbs > 2)
          ScaleRate = 10;
        else if (AlignRateAbs == 2)
          ScaleRate = 50;
        else
          ScaleRate = 250;

        if (AlignLoopCount > ScaleRate) {
          AlignLoopCount = 0;

          if (AlignRate >= 0)
            IncrAlignVal();
          else
            DecrAlignVal();

        }
      }

      SecDisp = (AlignValue + 15);  // Offset by 30 s to project *shadow* in the right place.
      if ( SecDisp > 29)
        SecDisp -= 30;

      MinDisp = SecDisp;
      HrDisp = (AlignValue + 6);  // Offset by 6 h to project *shadow* in the right place.

      if ( HrDisp > 11)
        HrDisp -= 12;
    }
    else if (OptionMode) {  // Option setting mode

#define StartOptTimeLimit 30
        if (StartingOption < StartOptTimeLimit) {

        AlignLoopCount++;  // Borrowing a counter variable...

        if (AlignLoopCount > 3) {
          AlignLoopCount = 0;
          StartingOption++;

          if (OptionMode == 1)  // Red (upper) ring color balance
          {
            HrDisp++;
            if (HrDisp > 11)
              HrDisp = 0;
          }
          if (OptionMode == 2)  // Green (middle) ring color balance
          {
            MinDisp++;
            if (MinDisp > 29)
              MinDisp = 0;
          }
          if (OptionMode == 3)  // Blue (lower) ring color balance
          {
            SecDisp++;
            if (SecDisp > 29)
              SecDisp = 0;
          }
          if (OptionMode >= 4)  // CW vs CCW OR fade mode
          {
            StartingOption = StartOptTimeLimit;  // Exit this loop
          }
        }
      }  // end "if (StartingOption < StartOptTimeLimit){}"

      if (StartingOption >= StartOptTimeLimit) {

        if (OptionMode == 4)
        {
          MinDisp++;
          if (MinDisp > 29)
            MinDisp = 0;
          SecDisp = MinDisp;
        }
        else
          normalTimeDisplay();

      }
    }
    else    {  // Regular clock display

      normalTimeDisplay();

    }

    h3 = HrDisp;
    l3 = HrNext;
    h4 = MinDisp;
    l4 = MinNext;
    h5 = SecDisp;
    l5 = SecNext;

    if (CCW){
      // Counterclockwise
      if (HrDisp)
        h3 = 12 - HrDisp;
      if (HrNext)
        l3 = 12 - HrNext;
      if (MinDisp)
        h4 = 30 - MinDisp;
      if (MinNext)
        l4 = 30 - MinNext;
      if (SecDisp)
        h5 = 30 - SecDisp;
      if (SecNext)
        l5 = 30 - SecNext;

      // Serial.print(HrDisp,DEC);
      // Serial.print(", ");
      // Serial.println(h3,DEC);

    }

    h0 = HrHi[h3];
    l0 = HrLo[h3];

    h1 = HrHi[l3];
    l1 = HrLo[l3];

    h2 = MinHi[h4];
    l2  = MinLo[h4];

    h3 = MinHi[l4];
    l3  = MinLo[l4];

    h4 = SecHi[h5];
    l4  = SecLo[h5];

    h5 = SecHi[l5];
    l5 = SecLo[l5];

  }

  SecFade2 = 0;
  SecFade1 = 63;

  MinFade2 = 0;
  MinFade1 = 63;

  HrFade2 = 0;
  HrFade1 = 63;

  if (SettingTime)  // i.e., if (SettingTime is nonzero)
  {

    HrFade1 = 5;
    MinFade1 = 5;
    SecFade1 = 5;

    if (SettingTime == 1)  // hours
    {
      HrFade1  = tempfade;
    }
    if (SettingTime == 2)  // minutes
    {
      MinFade1  = tempfade;
    }
    if (SettingTime == 3)  // seconds
    {
      SecFade1 = tempfade;
    }

  }
  else if (AlignMode + OptionMode)  // if either...
  {
    HrFade1 = 0;
    MinFade1 = 0;
    SecFade1 = 0;

    if (AlignMode){
      if (AlignMode < 3)
        SecFade1 = tempfade;
      else if (AlignMode > 4)
        HrFade1 = tempfade;
      else
        MinFade1 = tempfade;
    }
    else{  // Must be OptionMode....
      if (StartingOption < StartOptTimeLimit)
      {
        if (OptionMode == 1)
        {
          HrFade1 = tempfade;
        }
        if (OptionMode == 2)
        {
          MinFade1 = tempfade;
        }
        if (OptionMode == 3)
        {
          SecFade1 = tempfade;
        }
        if (OptionMode == 4)  // CW vs CCW
        {
          SecFade1 = tempfade;
          MinFade1 = tempfade;
        }
      }
      else {  // No longer in starting mode.

        HrFade1 = tempfade;
        MinFade1 = tempfade;
        SecFade1 = tempfade;

        if (OptionMode == 4)  // CW vs CCW
        {
          HrFade1 = 0;
        }
        else
        normalFades();
      }
    }

  }
  else
  {
    normalFades();

  }

  byte tempbright = MainBright;

  if (SleepMode)
    tempbright = 0;

  if (VCRmode){
    if (SecNow & 1)
      tempbright = 0;
  }

  d0 = HourBright*HrFade1*tempbright >> 7;
  d1 = HourBright*HrFade2*tempbright >> 7;
  d2 = MinBright*MinFade1*tempbright >> 7;
  d3 = MinBright*MinFade2*tempbright >> 7;
  d4 = SecBright*SecFade1*tempbright >> 7;
  d5 = SecBright*SecFade2*tempbright >> 7;

  // unsigned long  temp = millis();

  // This is the loop where we actually light up the LEDs:
  byte i = 0;
  while (i < 128)  // 128 cycles: ROUGHLY 39 ms  => Full redraw at about 3 kHz.
  {

    if (d0 > 0){
      TakeHigh(h0);
      TakeLow(l0);
      delayTime(d0);
      AllLEDsOff();
    }

    if (d1 > 0){
      TakeHigh(h1);
      TakeLow(l1);
      delayTime(d1);
      AllLEDsOff();
    }

    if (d2 > 0){
      TakeHigh(h2);
      TakeLow(l2);
      delayTime(d2);
      AllLEDsOff();
    }

    if (d3 > 0){
      TakeHigh(h3);
      TakeLow(l3);
      delayTime(d3);
      AllLEDsOff();
    }

    if (d4 > 0){
      TakeHigh(h4);
      TakeLow(l4);
      delayTime(d4);
      AllLEDsOff();
    }

    if (d5 > 0){
      TakeHigh(h5);
      TakeLow(l5);
      delayTime(d5);
      AllLEDsOff();
    }

    if (MainBright < 8){
      delayTime((8-MainBright)<<5);
      delayTime((8-MainBright)<<5);
      delayTime((8-MainBright)<<5);
    }

    i++;
  }

  /*
  temp = millis() - temp;
   Serial.println(temp,DEC);
  */

  // Can this sync be tried only once per second?
  if( getPCtime()) {  // try to get time sync from pc

    // Set time to that given from PC.
    // MinNow = DateTime.Minute;
    // SecNow = DateTime.Second;
    // HrNow = DateTime.Hour;
    MinNow = minute();
    SecNow = second();
    HrNow = hour();

    if ( HrNow > 11)  // Convert 24-hour mode to 12-hour mode
      HrNow -= 12;

    // Print confirmation
    Serial.println("Clock synced at: ");
    // Serial.println(DateTime.now(),DEC);
    Serial.println(now(),DEC);

    // if(DateTime.available()) {  // update clocks if time has been synced
    if(timeStatus()) {  // update clocks if time has been synced

      // if ( prevtime != DateTime.now() )
      if ( prevtime != now() )
      {
        if (ExtRTC)
          RTCsetTime(HrNow,MinNow,SecNow);

        // DateTime.available();  //refresh the Date and time properties
        timeStatus();  //refresh the Date and time properties
        digitalClockDisplay( );  // update digital clock
        // prevtime = DateTime.now();
        prevtime = now();
      }
    }
  }
}

