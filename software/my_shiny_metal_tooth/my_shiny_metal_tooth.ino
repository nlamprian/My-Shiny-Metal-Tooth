/**
 * Description:
 *   This is the software part of the "My-Shiny-Metal-Tooth" project.
 *   "My-Shiny-Metal-Tooth" is an illuminated sign for a dental clinic.
 *   It uses EL-Wire for lighting and an RTC module for timekeeping.
 *   It computes the sunset time for each day at which point the sign 
 *   turns on, and turns off later at a predetermined time.
 * 
 * Note 1:
 *   The DS1307 doesn't account for daylight saving time. But the TimeLord library
 *   allows to adjust a given time to DST. So, the code actually does take into
 *   account DST. There is no need for any manual time updates.
 * Note 2:
 *   The time on the RTC can shift over time. If you notice a deviation building up,
 *   just uncomment the last line in "setup" and load the example to your Arduino. 
 *   That will set the computer's time on the RTC. Afterwards, make sure to reupload 
 *   the code with the same line commented out. If you don't do that, the next time 
 *   your Arduino resets, it will write the time again on the RTC... 
 *   the time of the code's compilation.
 * Update:
 *   An LED strip has been added that shines light down the sign from above.
 * 
 * Author:
 *   Nick Lamprianidis <nlamprian@gmail.com>
 * 
 * Github repository:
 *   https://github.com/nlamprian/My-Shiny-Metal-Tooth
 * 
 * License:
 *   Copyright (c) 2015 Nick Lamprianidis 
 *   This code is released under the MIT license
 *   http://www.opensource.org/licenses/mit-license.php
 */

#include <Wire.h>  // Required by RTClib
#include <RTClib.h>
#include "TimeLord.h"

#define DEBUG

#define LED_STRIP  9
#define GREEN_WIRE 5
#define BLUE_WIRE  6

#define TURN_OFF_TIME (23 * 60l)  // 11:00 PM

// System States:
//  S_OFF  :  Sign is turned off
//  S_ON   :  Sign is turned on
//  L_ON   :  Sign is turned on and the lighting is on
//  L_BLINK:  Sign is turned on and the lighting is blinking
//  L_FADE :  Sign is turned on and the lighting is fading in/out
enum FSM { S_OFF, S_ON, L_ON, L_BLINK, L_FADE };

uint8_t state = S_OFF;


// Creates an RTC_DS1307 instance
// Handles the DS1307 Real-Time Clock
RTC_DS1307 RTC;

DateTime now;  // Holds the current date and time information

TimeLord tl;  // Computes the sunrise and sunset times

// Holds the current, and sunset, 
// date and time information
tlTime tlNow, tlSunset;


void setup ()
{
    #ifdef DEBUG
    Serial.begin (9600);
    while (!Serial) ;
    Serial.println ("My-Shiny-Metal-Tooth\n");
    #endif

    pinMode (LED_STRIP, OUTPUT);
    pinMode (GREEN_WIRE, OUTPUT);
    pinMode (BLUE_WIRE, OUTPUT);
    
    // Initialize RTC
    Wire.begin ();
    RTC.begin ();
    
    // Initialize TimeLord
    // Set local configuration
    tl.position (40.55f, 22.25f);
    tl.timeZone (2 * 60);
    // DST starts at the last Sunday of March and ends at the last 
    // Sunday of October. But since those Sundays are either the 4th or 
    // 5th of the month, I settle and just declare them as 4th.
    tl.dstRules (2, 3, 9, 3, 60);
    
    // Uncomment this to set the current time on the RTC module
    // RTC.adjust (DateTime (__DATE__, __TIME__));
}


void loop ()
{
    now = RTC.now ();
    tlNow = DT2TL (now);
    tl.dst ((uint8_t *) &tlNow);

    tlSunset = DT2TL (now);
    tl.sunSet ((uint8_t *) &tlSunset);

    unsigned long mNow = TL2Minutes (tlNow);
    unsigned long mSunset = TL2Minutes (tlSunset);

    #ifdef DEBUG
    printDateTime (now);
    printTLTime (tlNow, "tlTime:");
    printTLTime (tlSunset, "tlSunset:");

    Serial.println (String ("Now          : ") + mNow);
    Serial.println (String ("Sunset Time  : ") + mSunset);
    Serial.println (String ("Turn off time: ") + TURN_OFF_TIME);
    Serial.println ();
    #endif

    uint8_t rNum;
    switch (state)
    {
        case S_OFF:
            if (mNow > mSunset && mNow < TURN_OFF_TIME)
                state = S_ON;

            analogWrite (LED_STRIP, 0);
            digitalWrite (GREEN_WIRE, LOW);
            digitalWrite (BLUE_WIRE, LOW);
            break;
        case S_ON:
            if (mNow < mSunset || mNow > TURN_OFF_TIME)
            {
                state = S_OFF;
                break;
            }
            
            analogWrite (LED_STRIP, 120);
            
            rNum = rand () % 12;
            if (rNum < 6)      state = L_ON;     // 50%
            else if (rNum < 9) state = L_BLINK;  // 25%
            else               state = L_FADE;   // 25%
            
            break;
        case L_ON:
            wireOn ();
            state = S_ON;
            break;
        case L_BLINK:
            wireBlink ();
            state = S_ON;
            break;
        case L_FADE:
            wireFade ();
            state = S_ON;
            break;
    }

    #ifdef DEBUG
    Serial.println (String ("State: ") + state);
    Serial.println ("========================");
    Serial.println ();
    #endif

    if (state == S_OFF)
        delay (60 * 1000l);  // 1 min
}


void wireOn ()
{  // 20 s duration
    digitalWrite (GREEN_WIRE, HIGH);
    digitalWrite (BLUE_WIRE, HIGH);
    delay (20 * 1000l);
}


void wireBlink ()
{  // 20.8 s duration
    digitalWrite (GREEN_WIRE, HIGH);
    for (uint8_t i = 0; i < 13; ++i)
    {
        digitalWrite (BLUE_WIRE, HIGH);
        delay (800l);  // 0.8 s
        digitalWrite (BLUE_WIRE, LOW);
        delay (800l);  // 0.8 s
    }
}


void wireFade ()
{  // 18.9 s duration
    unsigned long t = millis ();
    int16_t dc = 0;
    bool dir = true;  // Up: true, Down: false

    digitalWrite (GREEN_WIRE, HIGH);

    uint16_t period = 100;  // ms
    uint8_t step = 20;
    uint16_t tSteps = 2 * (255 / step) + 3;
    uint16_t limit = (uint16_t) (20.f / (tSteps * period / 1000.f)) * tSteps;
    for (uint8_t i = 0; i < limit; ++i)
    {
        dc = dc + (dir ? step : -step);
        if (dc > 255)
        {
            dc = 255;
            dir = !dir;
        }
        else if (dc < step)
        {
            dc = step;
            dir = !dir;
        }

        analogWrite (BLUE_WIRE, dc);
        delay (period);  // 0.1 s
    }
}


unsigned long TL2Minutes (tlTime tl_)
{
    return tl_.hour * 60 + tl_.min;
}


tlTime DT2TL (DateTime &dt)
{
    tlTime tl_;
    tl_.sec = (uint8_t) dt.second ();
    tl_.min = (uint8_t) dt.minute ();
    tl_.hour = (uint8_t) dt.hour ();
    tl_.day = (uint8_t) dt.day ();
    tl_.month = (uint8_t) dt.month ();
    tl_.year = (uint8_t) (dt.year () - 2000);
    return tl_;
}


#ifdef DEBUG
void printDateTime (const DateTime &dt)
{
    const char* dayName[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
    const char* monthName[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
    
    Serial.println ("DateTime:");
    Serial.println (String (dayName[dt.dayOfWeek ()]) + " " + dt.day () + " " +
                   monthName[dt.month () - 1] + " " + dt.year ());
    Serial.println (String (dt.hour ()) + ":" + dt.minute () + ":" + dt.second ());
    Serial.println ();
}


void printTLTime (const tlTime &tl_, String title)
{
    Serial.println (title);
    Serial.println (String (tl_.day) + '/' + tl_.month + '/' + (2000 + tl_.year));
    Serial.println (String (tl_.hour) + ':' + tl_.min + ':' + tl_.sec);
    Serial.println ();
}
#endif
