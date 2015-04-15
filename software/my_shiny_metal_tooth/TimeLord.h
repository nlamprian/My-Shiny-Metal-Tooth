/** Original library by swfltek.com at http://swfltek.com/arduino/timelord.html
 *  
 *  A slight modification has been made in the include directives, 
 *  and the tlTime struct has been added.
 *  The library is not very accurate, but works nonetheless.
 */

#include <stdint.h>

#define tl_second 0
#define tl_minute 1
#define tl_hour   2
#define tl_day    3
#define tl_month  4
#define tl_year   5


typedef struct
{
    uint8_t sec;    // 0-59
    uint8_t min;    // 0-59
    uint8_t hour;   // 0-23
    uint8_t day;    // 1-31
    uint8_t month;  // 1-12
    uint8_t year;   // 0-99
} tlTime;


class TimeLord
{
public:
    TimeLord ();

    // Configuration
    bool position (float lat, float lon);
    bool timeZone (int tz);
    bool dstRules (uint8_t sMonth, uint8_t sSunday, uint8_t eMonth, uint8_t eSunday, uint8_t advance);
    
    // Political
    void gmt (uint8_t *now);
    void dst (uint8_t *now);
            
    // Solar & Astronomical
    bool sunRise (uint8_t *when);
    bool sunSet (uint8_t *when);
    float moonPhase (uint8_t *when);
    void sidereal (uint8_t *when, bool local);
    uint8_t season (uint8_t *when);
    
    // Utility
    uint8_t dayOfWeek (uint8_t *when);
    uint8_t lengthOfMonth (uint8_t *when);
    bool isLeapYear (int yr);
    
private:
    float latitude, longitude;
    int timezone;
    uint8_t dstm1, dstw1, dstm2, dstw2, dstadv;
    void adjust (uint8_t *when, long offset);
    bool computeSun (uint8_t *when, bool rs);
    uint8_t signum (int n);
    int absolute (int n);
    long dayNumber (uint16_t y, uint8_t m, uint8_t d);
    bool inDst (uint8_t *p);
    uint8_t _season (uint8_t *when);

};
