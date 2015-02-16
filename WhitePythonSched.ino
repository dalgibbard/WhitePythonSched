/*
 * Name:	WhitePythonSched-withLCD_v1.3
 * Author:      Darren Gibbard
 * Original Name:      LightController.ino
 * Original Author:    User "benjaf" at plantedtank.net forums
 * URL:		https://github.com/benjaf/LightController
 * This is an edited version of Benjaf's original code, originally modified for replacing
 *     the original Arcadia Classica OTL LED Controller Unit (which sucked.) and now, to replace a manual
 *     dimmer for the WhitePython LED  lighting series for Snake Vivariums.
 * NOTE: This code has been slightly mangled for the sake of a single channel.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. 
 */

/*
 =================
 |  ** DETAIL ** |
 =================
*/
// ----------------------- RTC Library -----------------------
// Use Wire and RTClib (https://github.com/mizraith/RTClib/):
// Please note that there are a significant differences between the original JeeLabs RTCLib and the Adafruit fork!
#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <RTC_DS3231.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include "ChannelManager.h"

// ----------------------- Constants -----------------------

/// *** SCRIPT VERSION *** ///
const char* ver = "v1.3";


const int MaxChannels = 1;   // Max number of channels, change if more or less are required
const int MaxPoints = 20;    // Max number of light intensity points, change if more or less are required

#define EMBEDDED_LED_PIN    13
byte ledState = LOW;
#define I2C_ADDR      0x27 // I2C address of PCF8574A
#define BACKLIGHT_PIN 3
#define En_pin        2
#define Rw_pin        1 
#define Rs_pin        0
#define D4_pin        4
#define D5_pin        5
#define D6_pin        6
#define D7_pin        7
LiquidCrystal_I2C twilcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin, BACKLIGHT_PIN, POSITIVE);

// Change the below to "True" if you need to set the RTC time on next upload.
// NOTE: Be sure to Disable again immediately after, and re-Upload, else time will be errornously set on every restart
//       of the Arduino!!
const char* setTime = "False";

// ----------------------- Variables -----------------------
// RTC
RTC_DS3231 RTC;

// Time
DateTime CurrentTime;

// secs for Time Display
int secs = 0;
int intcheck = 0;
int lvl = 0;
float lvlfloat = 0.0;
float percent;
int channel = 0;

// ----------------------- Lights -----------------------

// Schedule Point format: (Hour, Minute, Intensity)
// Difference in intensity between points is faded/increased gradually
// Example: 
// 	Channels[0].AddPoint(8, 0, 0);
//	Channels[0].AddPoint(8, 30, 255);
//	Channels[0].AddPoint(11, 0, 255);
//  ...
//
// Explanation:
//  00:00 - 08:00 -> Light OFF
//  08:00 - 08:30 -> Increment light towards Fully ON
//  08:30 - 11:00 -> Light Fully ON
//
// Min intensity value: 0 (OFF)
// Max intensity value: 255 (Fully ON)
//
// If only 1 point is added, channel always maintains value of that point
//
// There is the option of which fade mode to use.
// Basically there are 2 modes: Linear and Exponential
// - Linear is what you would expect - 50% on = 50% duty cycle.
// - Exponential is an estimation of how our eyes react to brightness. The 'real' formula is actually inverse logarithmic,
// 	 but for our use exponential is close enough (feel free to add more fade modes if you disagree) - and much faster to compute!

Channel Channels[MaxChannels];
Point Points[MaxChannels][MaxPoints];

void InitializeChannels(int channels) {
  
  /* ======================
      *** IMPORTANT NOTE:
     ======================
             * Valid power levels are 0 - 255 in the code below.
            * It is advisable to use the following Number Ideals:
                * OFF  = 0
                * 10%  = 26
                * 25%  = 64
                * 50%  = 128
                * 75%  = 192
                * 100% = 255
  */
  
	// Channel 0
	int channelNo = 0;	// Currently editing channel 0
	int pin = 9;		// Channel 0 uses pin 9
	Channels[channelNo] = Channel(pin, MaxPoints, fademode_linear, Points[channelNo]);	// Initialize channel and choose FadeMode
	Channels[channelNo].AddPoint(6, 30, 0);
        Channels[channelNo].AddPoint(7, 30, 5);
	Channels[channelNo].AddPoint(8, 30, 51);
	Channels[channelNo].AddPoint(11, 0, 255);
	Channels[channelNo].AddPoint(15, 0, 255);
	Channels[channelNo].AddPoint(18, 30, 51);
        Channels[channelNo].AddPoint(19, 15, 5);
	Channels[channelNo].AddPoint(20, 0, 0);

}

// ----------------------- Functions -----------------------
long lastUpdateTime = 0;

//override printf output
int my_putc(char outtext, FILE *t){
  Serial.print( outtext );
  twilcd.print( outtext );
};

// Update light intensity values
void UpdateLights(DateTime currentTime)
{
	long now = Seconds(currentTime.hour(), currentTime.minute(), currentTime.second());	// Convert current time to seconds since midnight
	if(now != lastUpdateTime)  	// Perform update only if there is a perceivable change in time (no point wasting clock cycles otherwise)
	{
            channel = 0;
            lvl = Channels[channel].GetLightIntensityInt(now);
            // Set the PWM Duty Cycle
            analogWrite(Channels[channel].GetPin(), lvl);	// Get updated light intensity and write value to pin (update is performed when reading value)
            lastUpdateTime = now;
       }
}

void PrintTimeToSerial (DateTime currentTime){
        // Print the Time now:
        if (int(currentTime.second()) != int(secs)){
           if (currentTime.day() < 10){
               Serial.print("0");
           }
           Serial.print(currentTime.day(), DEC);
           Serial.print('/');
           if (currentTime.month() < 10){
               Serial.print("0");
           }
           Serial.print(currentTime.month(), DEC);
           Serial.print('/');
           Serial.print(currentTime.year(), DEC);
           Serial.print(' ');
           if (currentTime.hour() < 10){
               Serial.print("0");
           }
           Serial.print(currentTime.hour(), DEC);
           Serial.print(':');
           if (currentTime.minute() < 10){
               Serial.print("0");
           }
           Serial.print(currentTime.minute(), DEC);
           Serial.print(':');
           if (currentTime.second() < 10){
               Serial.print("0");
           }
           Serial.print(currentTime.second(), DEC);
           Serial.println();
           secs = currentTime.second();
        }
}

// Convert HH:mm:ss -> Seconds since midnight
long Seconds(int hours, int minutes, int seconds) {
	return ((long)hours * 60 * 60) + (minutes * 60) + seconds ;
}

// ----------------------- Setup -----------------------
void setup() {        
        
  fdevopen( &my_putc, 0 ); //override printf output, this allows to print to more than one output device
  
  // Setup Serial Interface
  Serial.begin(9600);
       
  // Setup onboard LED
  pinMode(EMBEDDED_LED_PIN, OUTPUT);
       
  // Setup LCD
  twilcd.begin(20,4);
  twilcd.clear();
  twilcd.home();
  twilcd.setCursor (0,0);
  twilcd.print("********************");
  twilcd.setCursor (0,1);
  twilcd.print("  WhitePythonSched  ");
  twilcd.setCursor (0,2);
  twilcd.print("   FingAZ -- ");
  twilcd.print(ver);
  twilcd.setCursor (0,3);
  twilcd.print("********************");     
        
  // LED TEST + Delay
  digitalWrite(EMBEDDED_LED_PIN, HIGH);
  delay(250);
  digitalWrite(EMBEDDED_LED_PIN, LOW);
  delay(250);
  digitalWrite(EMBEDDED_LED_PIN, HIGH);
  delay(250);
  digitalWrite(EMBEDDED_LED_PIN, LOW);
  delay(250);
  digitalWrite(EMBEDDED_LED_PIN, HIGH);
  delay(250);
  digitalWrite(EMBEDDED_LED_PIN, LOW);
  delay(250);
  digitalWrite(EMBEDDED_LED_PIN, HIGH);
  delay(250);
  digitalWrite(EMBEDDED_LED_PIN, LOW);
  delay(250);
  digitalWrite(EMBEDDED_LED_PIN, HIGH);
  delay(250);
  digitalWrite(EMBEDDED_LED_PIN, LOW);
  delay(250);
  digitalWrite(EMBEDDED_LED_PIN, HIGH);
  delay(250);
  digitalWrite(EMBEDDED_LED_PIN, LOW);
  delay(250);
        
  twilcd.clear();
      
  // Initialize channel schedules
  InitializeChannels(MaxChannels);

  // Clock
  Wire.begin();
  RTC.begin();
    
  // If the user needs to set the RTC Time and has set the Var at the start, do it:
  if( setTime == "True" ){
    RTC.adjust(DateTime(__DATE__, __TIME__));  // Set RTC time to sketch compilation time, only use for 1 (ONE) run.
  }
}

// ----------------------- Loop -----------------------
void loop() {
	// Get current time
	CurrentTime = RTC.now();

        // Update lights (set lvl too for us to use)
	UpdateLights(CurrentTime);

        // For Debugging Time based issues Via Serial
        PrintTimeToSerial(CurrentTime);

        Serial.println(lvl);
        
        // Map lvl to a float without remapping the original var itself (else it messes up future calcs)
        Serial.print("Light Power: ");
        int newlvl = lvl;
        lvlfloat = lvl;
        percent = (lvlfloat / 255 ) * 100;
        Serial.print(percent);
        Serial.println("%");
        
        // Set out Time/Date display values.
        int nowHour = CurrentTime.hour();
        int nowMin = CurrentTime.minute();
        int nowDay = CurrentTime.day();
        int nowMonth = CurrentTime.month();
        
        // LCD Print
        /// Print Code info on Line0
        //twilcd.setCursor (0,0);
        //twilcd.print("Hedwig+Bella's Light");
        
        /// PRINT THE TIME Line 1 - slightly indented.
        twilcd.setCursor (3,0);
        if(nowHour < 10 ){
          twilcd.print("0");
          twilcd.print(nowHour);
        } else {
          twilcd.print(nowHour);
        }
        
        // Print Time Seperator
        twilcd.print(":");
            
        if(nowMin < 10 ){
          twilcd.print("0");
          twilcd.print(nowMin);
        } else {
         twilcd.print(nowMin);
        }

        /// PRINT THE DATE Line 1 - after Time                
        twilcd.setCursor (12,0);
        if(nowDay < 10 ){
          twilcd.print("0");
          twilcd.print(nowDay);
        } else {
          twilcd.print(nowDay);
        }
                
        twilcd.print("/");
        if(nowMonth < 10 ){
          twilcd.print("0");
          twilcd.print(nowMonth);
        } else {
          twilcd.print(nowMonth);
        }
         
        /// Print Generic level info on line 2:
        twilcd.setCursor (0,2);
        twilcd.print("Power Level: ");
        twilcd.print(percent);
        if ( percent != 100.00 ){ 
          twilcd.print("% ");
        } else {
          twilcd.print("%");
        } 
        
        /// Print Bar Graph on Line 3:
        // 20 chars, 
        twilcd.setCursor (0,3);
        if ( newlvl < 1 ){
          twilcd.print("|                  |");
        } else if ( newlvl <= 15 ) {
          twilcd.print("|=                 |");        
        } else if ( newlvl <= 30 ) {
          twilcd.print("|==                |");
        } else if ( newlvl <= 44 ) {
          twilcd.print("|===               |");
        } else if ( newlvl <= 59 ) {
          twilcd.print("|====              |");
        } else if ( newlvl <= 74 ) {
          twilcd.print("|=====             |");
        } else if ( newlvl <= 89 ) {
          twilcd.print("|======            |");
        } else if ( newlvl <= 103 ) {
          twilcd.print("|=======           |");
        } else if ( newlvl <= 118 ) {
          twilcd.print("|========          |");
        } else if ( newlvl <= 123 ) {
          twilcd.print("|=========         |");
        } else if ( newlvl <= 147 ) {
          twilcd.print("|==========        |");
        } else if ( newlvl <= 162 ) {
          twilcd.print("|===========       |");
        } else if ( newlvl <= 177 ) {
          twilcd.print("|============      |");
        } else if ( newlvl <= 191 ) {
          twilcd.print("|=============     |");
        } else if ( newlvl <= 206 ) {
          twilcd.print("|==============    |");
        } else if ( newlvl <= 220 ) {
          twilcd.print("|===============   |");
        } else if ( newlvl <= 235 ) {
          twilcd.print("|================  |");
        } else if ( newlvl < 249 ) {
          twilcd.print("|================= |");
        } else if ( newlvl = 255 ) {
          twilcd.print("|==================|");
        } else {
          twilcd.print("????????????????????");
        }
        
        delay(1000);
        
}
