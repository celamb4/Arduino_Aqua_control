#include "Wire.h"                 // library used for I2C
#include <LiquidCrystal_I2C.h>    // library used
#include <avr/wdt.h>              //watchdog timer to avoid infinite loop
#include <OneWire.h>
#include <DallasTemperature.h>

/***************************************/
/***************************************/
/******  Saturday April 5, 2014  *******/
/******     Aqua control        ********/
/***************************************/
/***************************************/



OneWire  ds(50);

#define DS1307_I2C_ADDRESS 0x68   // address used for clock
#define ONE_WIRE_BUS 50
#define TEMPERATURE_PRECISION 9

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
// arrays to hold device addresses
DeviceAddress tankThermometer = {0x28, 0x23, 0xF7, 0xF6, 0x04, 0x00, 0x00, 0xE0};
DeviceAddress lightThermometer = {0x28, 0x3C, 0xF3, 0xF7, 0x04, 0x00, 0x00, 0x0F};

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); //set the address for the LCD_I2C 

/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                 R E L A Y   P A R T                                                              * */
/* *                                 S I M P L E   O N   A N D   O F F   F E A T U R E                                * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */  

int relayState1 = LOW;             
int relay_8 = 22; 
int relay_7 = 24; 
int relay_6 = 26; 
int relay_5 = 28; 
int relay_4 = 30; //Wavemaker
int relay_3 = 32; //Driver2
int relay_2 = 34; //Driver1
int relay_1 = 36; //Fans
int relay_vss = 38;

long previousMillis1 = 0;        

long interval1 = 15000;           // how many milliseconds to turn it on and off for RELAY1

//TEMP Variables
float tempC_tank, tempC_led;
int reading_tank, reading_led;
int mt_tank = 26.5; //max temp, turns on sump fan
int min_tank = 25.0; //min temp for sump fan to turn off
int mt_led = 40; //shuts down LEDs
int temp_vcc = 52;

/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                 L E D   D I M M I N G   P A R T                                                  * */
/* *                                 F A D E S   I N   A N D   O U T                                                  * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */



/* ********************************************************************************* */
/* *     									   * */
/* *     Note: time for ramp down is the same as time for ramp up.                 * */
/* *     									   * */
/* ********************************************************************************* */
int ontime =7;   // what time do you want the blue to start to ramp up?
int night = 19;
int blueramptime = 60 ;           // how many minutes do you want the blue to ramp up?
int whiteramptime = 120 ;         // after the blue, how many minutes do you want the white to ramp up?
int photoperiod = 360 ;           // after the white, how many minutes do you want it to stay at max intensity?
const int blue = 3;                     // which arduino pwm pin are you using for the blue?
const int white = 4;                   // which arduino pwm pin are you using for the white?
const int var = 2;                    //third PWM

int bluemin = 255 ;      //inverted logic    
int whitemin = 255 ; 
int varmin = 0 ;  //regular logic

int bluepercent[11] = { 255, 240, 225, 210, 195, 180, 165, 150, 135, 120, 100}; //inverted for PWM
int whitepercent[11] = { 255, 242, 229, 216, 203, 190, 177, 164, 151, 138, 125};  
int varpercent[11] = { 0, 26, 52, 78, 103, 128, 154, 180, 205, 230, 255};  //regular for PWM

int blueglobal = 0;
int whiteglobal = 0;

int inByte = 1;

/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                 R T C   C L O C K   D S 1 3 0 7                                                  * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */
byte decToBcd(byte val)           // Convert normal decimal numbers to binary coded decimal
{
  return ( (val/10*16) + (val%10) );
}


byte bcdToDec(byte val)           // Convert binary coded decimal to normal decimal numbers
{
  return ( (val/16*10) + (val%16) );
}

                                  // 1) Sets the date and time on the ds1307
                                  // 2) Starts the clock
                                  // 3) Sets hour mode to 24 hour clock
                                  // Assumes you're passing in valid numbers

void setDateDs1307(byte second,   // 0-59
byte minute,                      // 0-59
byte hour,                        // 1-23
byte dayOfWeek,                   // 1-7
byte dayOfMonth,                  // 1-28/29/30/31
byte month,                       // 1-12
byte year)                        // 0-99
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(second));    // 0 to bit 7 starts the clock
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));      // If you want 12 hour am/pm you need to set
                                  // bit 6 (also need to change readDateDs1307)
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

                                  // Gets the date and time from the ds1307
void getDateDs1307(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  // Reset the register pointer
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

  // A few of these need masks because certain bits are control bits
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f); // Need to change this if 12 hour am/pm
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}



/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                 D E F I N E  :  O N E S E C O N D                                                * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */

void onesecond() //function that runs once per second while program is running
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  lcd.setCursor(0, 0);
  if(hour>0)
  {
    if(hour<=12)
    {
      lcd.print(hour, DEC);
    }
    else
    {
      lcd.print(hour-12, DEC);
    }
  }
  else
  {
    lcd.print("12");
  }
  lcd.print(":");
  if (minute < 10) {
    lcd.print("0");
  }
  lcd.print(minute, DEC);
  lcd.print(":");
  if (second < 10) {
    lcd.print("0");
  }
  lcd.print(second, DEC);
  if(hour<12)
  {
    lcd.print("am");
  }
  else
  {
    lcd.print("pm");
  }
  lcd.print(" ");
  wdt_reset(); //every second bite back, to let the arduino know everything is running
  delay(1000);
}


//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||  D E F I N E  :  L U N A R P H A S E |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void moonPhase(int year, int month, int day)
{
  int ml[29] = {0,1.5,3,4.5,6,7.5,9,10.5,12,13.5,15,16,21,21,21,23,21,21,21,16,15,13.5,12,10.5,9,7.5,6,2,0}; //PWM values for moonlight channel, where 15 is FULL MOON

  int Y = (year-2000);
  int R = Y%19;
  int N = R-19;
  int n = N*11;

  while (n > 29 || n < -29){
    
    if (n > 29 && n > 0){
      n = n-30;
    }
    if (n < -29 && n < 0){
      n = n + 30;
    }
  }
  
  int t = n + day + month;
  int P = abs(t - 8); // current MoonPhase from 0-29, where 15 is full moon
  Serial.print("Var: ");
  Serial.println(ml[P]);
  
  if ( ((P >= 0) && (P <= 3)) || ((P >= 26) && (P <= 29))){		//new moon 
       analogWrite(var, ml[P]);
       lcd.setCursor(11,0);
       lcd.print("NM:");
       lcd.print(P);
  }
  else if ( ((P >= 4) && (P <= 7)) || ((P >= 23) && (P <= 25)) ){	//cresent
       analogWrite(var, ml[P]);
       lcd.setCursor(11,0);
       lcd.print("CM:");
       lcd.print(P);
  }
  else if ( ((P >= 8) && (P <= 11)) || ((P >= 19) && (P <= 22)) ){	//half moon
       analogWrite(var, ml[P]);
       lcd.setCursor(11,0);
       lcd.print("HM:");
       lcd.print(P);
   }
   else if ( ((P >= 12) && (P <= 15)) || ((P >= 16) && (P <= 19)) ){	//gibbous //full moon is full intensity 
       analogWrite(var, ml[P]);
       lcd.setCursor(11,0);
       lcd.print("FM:");
       lcd.print(P);
   }
    else  {                                                            //in case it doesnt find a case, show on LCD to correct code
       lcd.setCursor(15,0);
       lcd.print("ERROR");
       delay(500);
   }
   
}
  
//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||  D E F I N E  :  T E M P E R A T U R E |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
void temperature() //checks the LM35 temp and average_leds to smooth the value
{

  digitalWrite(temp_vcc, HIGH);
  
  sensors.requestTemperatures();
  
  tempC_tank = sensors.getTempC(tankThermometer);
  tempC_led = sensors.getTempC(lightThermometer);

  lcd.setCursor(0,2);
  lcd.print("LED T:");
  lcd.print(tempC_led);
  lcd.print("  ");
  lcd.setCursor(0,3);
  lcd.print("Tank T:");
  lcd.print(tempC_tank);
  
  Serial.print("tank_: ");
  Serial.println(tempC_tank);
  Serial.print("led_t: ");
  Serial.println(tempC_led);
  
  Serial.print("Blue_: ");
  Serial.println(blueglobal);
  Serial.print("White: ");
  Serial.println(whiteglobal);
  
  if((tempC_tank >= mt_tank) && (tempC_tank != 85.00)) {
      digitalWrite(relay_5, LOW);
  }
  
  else if ((tempC_tank <= min_tank) && (tempC_tank != 85.00)) {
    digitalWrite(relay_5, HIGH);
  }
}

void raspi() //checks the LM35 temp and average_leds to smooth the value
{

  if (Serial.available()) {
    inByte = Serial.read() - '0';  
    
    if (inByte == 0){
      lcd.noBacklight();
    }
    else if (inByte ==1){
      lcd.backlight();
    }
    
    else if (inByte ==2){
      
      lcd.backlight();
      digitalWrite(relay_1, LOW);
      digitalWrite(relay_2, LOW);
      digitalWrite(relay_3, LOW);
      delay(500);
      
      analogWrite(blue, 215);
      analogWrite(white, 255);
      analogWrite(var, varmin);
      
      int countdown = 60;
        while (countdown>0)
        {
          onesecond(); // updates clock once per second
          countdown--;
          relay1();
        }
       
      lcd.noBacklight(); 
      inByte = 0;
  
    }
    
    else {
      delay(100);     //do nothing
  }
 }
}


void discoverOneWireDevices(void) {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  
  Serial.print("Looking for 1-Wire devices...\n\r");
  while(ds.search(addr)) {
    Serial.print("\n\rFound \'1-Wire\' device with address:\n\r");
    for( i = 0; i < 8; i++) {
      Serial.print("0x");
      if (addr[i] < 16) {
        Serial.print('0');
      }
      Serial.print(addr[i], HEX);
      if (i < 7) {
        Serial.print(", ");
      }
    }
    if ( OneWire::crc8( addr, 7) != addr[7]) {
        Serial.print("CRC is not valid!\n");
        return;
    }
  }
  Serial.print("\n\r\n\rThat's it.\r\n");
  ds.reset_search();
  return;
}


/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                 D E F I N E  :  R E L A Y 1                                                      * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */

void relay1()  			                //FUNCTION TO TURN ON AND OFF RELAY 1.
{ 
  
  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis1 > interval1) 
  { 
    temperature();
    previousMillis1 = currentMillis;   
    if (relayState1 == LOW){
      relayState1 = HIGH;
    }
    else
      relayState1 = LOW;
      digitalWrite(relay_4, relayState1);
  }
}

/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                 S E T U P                                                                        * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */

void setup() {
  
  pinMode(relay_4, OUTPUT);     // set the digital pin as output:
  pinMode(relay_1, OUTPUT); 
  pinMode(relay_2, OUTPUT);    
  pinMode(relay_3, OUTPUT); 
  pinMode(relay_5, OUTPUT);
  pinMode(relay_6, OUTPUT);
  pinMode(relay_7, OUTPUT);
  pinMode(relay_8, OUTPUT);
  pinMode(relay_vss, OUTPUT);
  pinMode(temp_vcc, OUTPUT); 
  
  pinMode(blue, OUTPUT);
  pinMode(white, OUTPUT); 
  pinMode(var, OUTPUT); 
  
  sensors.begin();
  sensors.setResolution(tankThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(lightThermometer, TEMPERATURE_PRECISION);
  
  wdt_enable(WDTO_2S); //watchdog timer value is set for 2 seconds, if the main loop doesnt complete in 2 seconds the arduino will reset
  delay(100);
  Serial.begin(9600);  //init serial for python interface
  
/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                 S E T U P - D I S P L A Y                                                        * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */

  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  Wire.begin();
  second = 0;
  minute = 10;
  hour = 17;
  dayOfWeek = 6;  		              // Sunday is 0
  dayOfMonth = 05;
  month = 04;
  year = 14;


/* ********************************************************************************* */
/* *                                                                               * */
/* *     this is where you set your time...                                        * */
/* *     1) change the second, minute, hour, etc above                             * */
/* *     2) remove // below                                                        * */
/* *     3) and load this sketch to your arduino                                   * */
/* *     4) after loading the sketch, put the // back again                        * */
/* *     5) and load this sketch again to your arduino, and save                   * */
/* *                                                                               * */
/* ********************************************************************************* */
//setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);


  analogWrite(blue, bluemin);
  analogWrite(white, whitemin);
  analogWrite(var, varmin);
  
  lcd.begin(20, 4); // set up the LCD's number of rows and columns: 
  lcd.backlight();
  
  lcd.setCursor(0, 1);
  lcd.print("Blue:S");
  lcd.print(0);
  lcd.setCursor(8, 1);
  lcd.print("White:S");
  lcd.print(0); 
  lcd.setCursor(4,4);
 
 
  digitalWrite(relay_1, HIGH); //inverted logic HIGH=OFF, turn off all relays
  digitalWrite(relay_2, HIGH);
  digitalWrite(relay_3, HIGH);
  digitalWrite(relay_4, HIGH);
  digitalWrite(relay_5, HIGH);
  digitalWrite(relay_6, HIGH);
  digitalWrite(relay_7, HIGH);
  digitalWrite(relay_8, HIGH);
  digitalWrite(temp_vcc, HIGH);
  
  digitalWrite(relay_vss, LOW); //ground for relay board.
  
delay(500);

  
}





/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                 L O O P                                                                          * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */




void loop()
{
  onesecond();
  //backlight();
  relay1();
  lcd.setCursor(0, 1);
  lcd.print("Blue:");
  lcd.setCursor(8, 1);
  lcd.print("White:");
  //discoverOneWireDevices();

/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                 L O O P - D I M   F U N C T I O N                                                * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */

  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  int daybyminute = ((hour * 60) + minute); //converts time of day to a single value in minutes
      

  int bluerampup;
     if (daybyminute >= (ontime*60)) 
       bluerampup = (((ontime*60) + blueramptime) - daybyminute);
     else
       bluerampup = blueramptime;
       
  int whiterampup;
    if (daybyminute >= (ontime*60 + blueramptime)) 
       whiterampup = (((ontime*60) + blueramptime + whiteramptime) - daybyminute);
     else
       whiterampup = whiteramptime;

  int whiterampdown;
    if (((ontime * 60) + photoperiod + blueramptime + whiteramptime) <= daybyminute)
      whiterampdown = (((ontime*60) + photoperiod + blueramptime + 2*whiteramptime) - daybyminute);
    else
      whiterampdown = whiteramptime;
      
  int bluerampdown;
    if (((ontime * 60) + photoperiod + blueramptime + 2*whiteramptime) <= daybyminute)
      bluerampdown = (((ontime*60) + photoperiod + 2*blueramptime + 2*whiteramptime) - daybyminute);
    else
      bluerampdown = blueramptime;



/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                 L O O P - F A D E  I N                                                           * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */
if ((tempC_led < mt_led) && (tempC_led != 85.00)){
 if (daybyminute >= (ontime*60))
  { 
    if (daybyminute <= ((ontime*60) + blueramptime + (whiteramptime - 1))) //if time is in range of fade in, start fading in + (whiteramptime/10*9)
    {
      
      digitalWrite(relay_1, LOW);
      digitalWrite(relay_2, LOW);
      digitalWrite(relay_3, LOW);
      
      // fade blue LEDs in from min to max.
      lcd.setCursor(14,0);
      lcd.print(" ");  
      lcd.print("RISE");
      for (int i = 1; i <= 10; i++) // setting ib value for 10% increment. Start with 0% 
      { 
          analogWrite(blue, bluepercent[i]);
          analogWrite(var, varpercent[i]); 
          lcd.setCursor(5, 1);
          lcd.print(i);
          lcd.print(" "); 
          lcd.setCursor(14, 1);
          lcd.print("0");
          
          blueglobal = bluepercent[i];
          whiteglobal = whitepercent[0];
          
          /*Serial.print("Blue_: ");
          Serial.println(bluepercent[i]);
          Serial.print("White: ");
          Serial.println(whitepercent[0]);
          */
          
          int countdown = ((bluerampup*60)/10); // calculates seconds to next step
          while (countdown>0)
          {
          onesecond(); // updates clock once per second
          //backlight();
          countdown--;
          relay1();
        }
      }      

      // fade white LEDs in from min to max.
      for (int i = 1; i <= 10; i++) // setting i value for 10% increment. Start with 0%
      { 
          analogWrite(white, whitepercent[i]); 
          lcd.setCursor(14, 1);
          lcd.print(i);
          lcd.print(" "); 
          lcd.setCursor(5, 1);
          lcd.print("10");
          
          blueglobal = bluepercent[10];
          whiteglobal = whitepercent[i];
          
          /*
          Serial.print("Blue_: ");
          Serial.println(bluepercent[10]);
          Serial.print("White: ");
          Serial.println(whitepercent[i]);
          */
          
          int countdown = ((whiterampup*60)/10); // calculates seconds to next step
          while (countdown>0)
          {
            onesecond(); // updates clock once per second
            //backlight();
            countdown--;
            relay1();
        }
      } 
    }
  }
}

  
/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                 L O O P - M A X  V A L U E                                                       * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */

if ((tempC_led < mt_led) && (tempC_led != 85.00)){
 if (daybyminute >= ((ontime * 60) + blueramptime + whiteramptime)) 
  { 
    if ( daybyminute <= ((ontime * 60) + blueramptime + whiteramptime + photoperiod)) // if time is in range of photoperiod, turn lights on to maximum fade value
    {

        digitalWrite(relay_1, LOW);
        digitalWrite(relay_2, LOW);
        digitalWrite(relay_3, LOW);
    
        lcd.setCursor(14,0);
        lcd.print(" "); 
        lcd.print(" "); 
        lcd.print("MAX");
        
        analogWrite(blue, bluepercent[10]);
        analogWrite(var, varpercent[10]);
        lcd.setCursor(5, 1);
        lcd.print(10);
        lcd.print(" ");
        analogWrite(white, whitepercent[10]); 
        lcd.setCursor(14, 1);
        lcd.print(10);
        lcd.print(" "); 
        
        blueglobal = bluepercent[10];
        whiteglobal = whitepercent[10];
        
        /*
        Serial.print("Blue_: ");
        Serial.println(bluepercent[10]);
        Serial.print("White: ");
        Serial.println(whitepercent[10]);
        */
        
        //backlight();
      
    } 
  }
}

/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                 L O O P - F A D E  O U T                                                         * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */

if ((tempC_led < mt_led) && (tempC_led != 85.00)){
  if (((ontime * 60) + photoperiod + blueramptime + whiteramptime) <= daybyminute)
  { 
    if (((ontime * 60) + photoperiod + whiteramptime + 2*blueramptime + (blueramptime/10*9)) >= daybyminute)
    {
     
      
      digitalWrite(relay_1, LOW);
      digitalWrite(relay_2, LOW);
      digitalWrite(relay_3, LOW);
   
      lcd.setCursor(14,0);
      lcd.print(" "); 
      lcd.print(" "); 
      lcd.print("SET");
      
      // fade white LEDs out from max to min in increments of 1 point:
      for (int i = 10; i >= 0; i--) // setting i value for 10% increment. Start with 10%
      { 
        analogWrite(blue, bluepercent[10]);
        analogWrite(var, varpercent[10]);
        lcd.setCursor(5, 1);
        lcd.print(10);
        lcd.print(" "); 
        
        analogWrite(white, whitepercent[i]); 
        lcd.setCursor(14, 1);
        lcd.print(i);
        lcd.print(" ");  
        
        blueglobal = bluepercent[10];
        whiteglobal = whitepercent[i];
        
        /*
        Serial.print("Blue_: ");
        Serial.println(bluepercent[10]);
        Serial.print("White: ");
        Serial.println(whitepercent[i]);
        */

        int countdown = ((whiterampdown*60)/10); // calculates seconds to next step
        while (countdown>0)
        {
          onesecond(); // updates clock once per second
          //backlight();
          countdown--;
          relay1();
        }

      } 

      // fade blue LEDs out from max to min in increments of 1 point:
      for (int i = 10; i >= 0; i--) // setting i value for 10% increment. Start with 10%
      { 
        analogWrite(blue, bluepercent[i]);
        analogWrite(var, varpercent[i]);
        lcd.setCursor(5, 1);
        lcd.print(i);
        lcd.print(" ");
        lcd.setCursor(14, 1);
        lcd.print("0");
        lcd.print(" "); 
        
        blueglobal = bluepercent[i];
        whiteglobal = whitepercent[0];
        
        /*
        Serial.print("Blue_: ");
        Serial.println(bluepercent[i]);
        Serial.print("White: ");
        Serial.println(whitepercent[0]);
        */

        int countdown = ((bluerampdown*60)/10); // calculates seconds to next step
        while (countdown>0)
        {
          onesecond(); // updates clock once per second
          //backlight();
          countdown--;
          relay1();
        }
      }
    } 
  }
}
/* ******************************************************************************************************************** */
/* *                                                                                                                  * */
/* *                                           N I G H T                                                              * */
/* *                                                                                                                  * */
/* ******************************************************************************************************************** */

   if((daybyminute >= ((night * 60) + 15)) || (((ontime*60) - 2) >= daybyminute)) {          
 
          digitalWrite(relay_1, HIGH);
          digitalWrite(relay_2, HIGH);
          digitalWrite(relay_3, HIGH);
          analogWrite(blue, 255);
          analogWrite(white, 255);
          analogWrite(var, varmin);
          
          blueglobal = bluepercent[0];
          whiteglobal = whitepercent[0];
          
          /*
          Serial.print("Blue_: ");
          Serial.println(bluepercent[0]);
          Serial.print("White: ");
          Serial.println(whitepercent[0]);
          */
          
          lcd.setCursor(14,0);
          lcd.print("NIGHT"); 

          lcd.setCursor(5, 1);
          lcd.print("N");
          lcd.print(" ");
          lcd.setCursor(14, 1);
          lcd.print("N");
          lcd.print(" "); 
          onesecond(); // updates clock once per second
          relay1(); 
          //backlight();
          //temperature();
          //moonPhase(year, month, dayOfMonth);

  
    }
    
    if((tempC_led >= mt_led) && (tempC_led != 85.00)) {   //shutdown lights and leave relay_1 ON, in order to reduce temp
      
          digitalWrite(relay_1, LOW); //Fans
          digitalWrite(relay_2, HIGH); //Driver1
          digitalWrite(relay_3, HIGH); //Driver2
          analogWrite(blue, 255);
          analogWrite(white, 255);
          analogWrite(var, 0);
          lcd.setCursor(14,0);
          lcd.print(" "); 
          lcd.print("HOT");
          lcd.setCursor(5, 1);
          lcd.print("H");
          lcd.setCursor(14, 1);
          lcd.print("H");
          temperature();                //keep checking the temp
          relay1();
          onesecond();
          //backlight();
          Serial.println("MAX TEMP");
          Serial.println("WARNING");
         
    }
    
  }  // END LOOP



