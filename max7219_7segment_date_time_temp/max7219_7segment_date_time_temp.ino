/**
 * @file        max7219_7segment_date_time_temp.c
 *
 * @brief       view DATE/TIME/TEMPERATURE using an 8-digit 7-Segment display
 *
 * @history     Feb 6th, 2017
 *
 * @author      Allan Schwartz <allans@CodeValue.net>
 *              Michael Diamond <m.diamond.il@gmail.com>
 *
 * @notes       The following Arduino Pins are attached:
 *
 *              name    Arduino   7 Segment display
 *              -----   ----      -----
 *                      +5V       VCC
 *                      GND       GND
 *              DIN     D11/MOSI  DIN
 *              CS      D10/SS    CS
 *              CLK     D13/SCK   CLK
 *
 *              name    Arduino   RTC module
 *              -----   ----      -----
 *                      nc        32K
 *                      nc        SQW
 *              SCL     SCL/A5    SCL
 *              SDA     SDA/A4    SDA
 *                      3v3       VCC
 *                      GND       GND
 */

#include <DS3231.h>           // import library from rinkydink.com

// define pins attached to MAX7219 (and also see notes above)
#define MAX7219_DIN           4
#define MAX7219_CS            5
#define MAX7219_CLK           6

// enumerate the MAX7219 registers
// See MAX7219 Datasheet, Table 2, page 7
enum {  MAX7219_REG_DECODE    = 0x09,
        MAX7219_REG_INTENSITY = 0x0A,
        MAX7219_REG_SCANLIMIT = 0x0B,
        MAX7219_REG_SHUTDOWN  = 0x0C,
        MAX7219_REG_DISPTEST  = 0x0F
     };

// enumerate the SHUTDOWN modes
// See MAX7219 Datasheet, Table 3, page 7
enum  { OFF = 0,
        ON  = 1
      };

const byte DP = 0b10000000;
const byte C  = 0b01001110;
const byte F  = 0b01000111;


// create an instance of the DS3231 called 'rtc',
// and specify the hardware interface PINS
DS3231 rtc(SDA, SCL);


// ... setup code here, to run once
void setup()
{
    // initialize the serial port:
    Serial.begin(115200);           // initialize serial communication
    Serial.println("\nmax7219_7segment_date_time_temp\n");

    // define type of pin
    pinMode(MAX7219_DIN, OUTPUT);   // serial data-in
    pinMode(MAX7219_CS, OUTPUT);    // chip-select, active low
    pinMode(MAX7219_CLK, OUTPUT);   // serial clock
    digitalWrite(MAX7219_CS, HIGH);

    resetDisplay();                 // reset the MAX2719 display

    rtc.begin();                    // initialize the DS3231 RTC interface

    // Uncomment the following line to set the RTC
    // then the RTC will be set to the compile time.

    // (so this needs to be just done just once), 
    // then comment out this line again and reprogram the Arduino
    
    //set_RTC_to_compile_time();
}


// ... set the RTC date and Time to the compile date and time
void set_RTC_to_compile_time()
{
    const char time_now[] = __TIME__;   // hh:mm:ss

    Serial.print("compile time: "); Serial.println(time_now);
    int hh = atoi(&time_now[0]);
    int mm = atoi(&time_now[3]);
    int ss = atoi(&time_now[6]);
    Serial.println("hh: " + String(hh) + " mm: " + String(mm) + " ss: " + String(ss));
    rtc.setTime(hh, mm, ss);        // Set the time to the compile time (24hr format)

    const char date_now[] = __DATE__;   // Mmm dd yyyy
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
                            
    Serial.print("compile date: "); Serial.println(date_now);
    int Mmm = 0;
    for ( int i = 0; i < 12; i++ ) {
        if (strncmp(date_now, months[i], 3) == 0 ) {
            Mmm = i + 1;
            break;
        }
    }

    int dd = atoi(&date_now[3]);
    int yyyy = atoi(&date_now[7]);
    Serial.println("Mmm: " + String(Mmm) + " dd: " + String(dd) + " yyyy: " + String(yyyy));
    rtc.setDate(dd, Mmm, yyyy);     // Set the date to dd-Mmm-yyyy
    rtc.setDOW();                   // Calculate and set Day-of-Week
}


// ... loop forever...
void loop()
{
    String str;                     // scratch or working string

    // ... display Date, dd.mm.yyyy
    // read the date as a string from the RTC
    str = rtc.getDateStr(FORMAT_LONG);
    //Serial.println(str);            // debug
    displayDate(str);               // display on the 7-segment
    delay(6000);

    // ... display Time, hh-mm-ss
    for ( int i = 0; i < 6; i++ ) {
        // read the time as a string from the RTC
        str = rtc.getTimeStr(FORMAT_LONG);
        //Serial.println(str);        // debug
        displayTime(str);           // display on the 7-segment
        delay(1000);
    }

    // ... display Temperature in Celsius, xx.xx C
    // read the temperature, as a float, from the RTC
    float t = rtc.getTemp();
    str = String(t, 2);             // format that value
    //Serial.println(str);            // debug
    displayTemp(str, C);            // display on the 7-segment
    delay(3000);

    // ... display Temperature in Fahrenheit, xx.xx F
    t = t * 1.8 + 32.0;             // convert the value to Fahrenheit
    str = String(t, 1);             // format that value
    //Serial.println(str);            // debug
    displayTemp(str, F);            // display on the 7-segment
    delay(3000);
}


// ... write a value into a max7219 register
// See MAX7219 Datasheet, Table 1, page 6
void set_register(byte reg, byte value)
{
    digitalWrite(MAX7219_CS, LOW);
    shiftOut(MAX7219_DIN, MAX7219_CLK, MSBFIRST, reg);
    shiftOut(MAX7219_DIN, MAX7219_CLK, MSBFIRST, value);
    digitalWrite(MAX7219_CS, HIGH);
}


// ... reset the max7219 chip
void resetDisplay()
{
    set_register(MAX7219_REG_SHUTDOWN, OFF);      // turn off display
    set_register(MAX7219_REG_DISPTEST, OFF);      // turn off test mode
    set_register(MAX7219_REG_INTENSITY, 0x0D);    // Set display intensity
}


// ... display the DATE on the 7-segment display
void displayDate(String dateString)
{
    set_register(MAX7219_REG_SHUTDOWN, OFF);      // turn off display
    set_register(MAX7219_REG_SCANLIMIT, 7);       // scan limit to 8 digits
    set_register(MAX7219_REG_DECODE, 0b11111111); // decode all 8 digits

    set_register(1, dateString.charAt(9));
    set_register(2, dateString.charAt(8));
    set_register(3, dateString.charAt(7));
    set_register(4, dateString.charAt(6));
    set_register(5, dateString.charAt(4) | DP);   // plus decimal point
    set_register(6, dateString.charAt(3));
    set_register(7, dateString.charAt(1) | DP);
    set_register(8, dateString.charAt(0));

    set_register(MAX7219_REG_SHUTDOWN, ON);       // Turn on the display
}


// ... display the TIME on the 7-segment display
void displayTime(String timeString)
{
    set_register(MAX7219_REG_SHUTDOWN, OFF);      // turn off display
    set_register(MAX7219_REG_SCANLIMIT, 7);       // scan limit to 8 digits
    set_register(MAX7219_REG_DECODE, 0b11111111); // decode all 8 digits

    set_register(1, timeString.charAt(7));
    set_register(2, timeString.charAt(6));
    set_register(3, timeString.charAt(5));
    set_register(4, timeString.charAt(4));
    set_register(5, timeString.charAt(3));
    set_register(6, timeString.charAt(2));
    set_register(7, timeString.charAt(1));
    set_register(8, timeString.charAt(0));

    set_register(MAX7219_REG_SHUTDOWN, ON);       // Turn on the display
}


// ... display the TEMP on the 7-segment display
void displayTemp(String tempString, char C_or_F )
{
    set_register(MAX7219_REG_SHUTDOWN, OFF);      // turn off display
    set_register(MAX7219_REG_SCANLIMIT, 5);       // scan limit to 6 digits
    set_register(MAX7219_REG_DECODE, 0b00111100); // decode 4 digits

    set_register(1, C_or_F);
    set_register(2, 0);                           // blank
    set_register(3, tempString.charAt(4));
    set_register(4, tempString.charAt(3));
    set_register(5, tempString.charAt(1) | DP);   // plus decimal point
    set_register(6, tempString.charAt(0));

    set_register(MAX7219_REG_SHUTDOWN, ON);       // Turn On display
}
