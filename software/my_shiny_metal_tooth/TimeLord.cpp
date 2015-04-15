/** Original library by swfltek.com at http://swfltek.com/arduino/timelord.html
 *  
 *  A slight modification has been made in the include directives, 
 *  and the tlTime struct has been added.
 *  The library is not very accurate, but works nonetheless.
 */

#include <math.h>
#include "TimeLord.h"


TimeLord::TimeLord ()
{
    latitude = 27.f;
    longitude = -82.f;
    timezone = -300;
    dstRules (3, 2, 11, 1, 60);  // USA
}


bool TimeLord::position (float lat, float lon)
{
    if (fabs (lon) > 180.0)
        return false;

    if (fabs (lat) > 90.0)
        return false;

    latitude = lat;
    longitude = lon;
    return true;
}


bool TimeLord::timeZone (int tz)
{
    if (absolute (tz) > 720)
        return false;
    
    timezone = tz;
    return true;
}


bool TimeLord::dstRules (uint8_t sMonth, uint8_t sSunday, uint8_t eMonth, uint8_t eSunday, uint8_t advance)
{
    if (sMonth == 0 || sSunday == 0 || eMonth == 0 || eSunday == 0)
        return false;
    
    if (sMonth > 12 || sSunday > 4 || eMonth > 12 || eSunday > 4)
        return false;
    
    dstm1 = sMonth;
    dstw1 = sSunday;
    dstm2 = eMonth;
    dstw2 = eSunday;
    dstadv = advance;
    return true;
}


void TimeLord::gmt (uint8_t * now)
{
    adjust (now, -timezone);
}


void TimeLord::dst (uint8_t *now)
{
    if (inDst (now))
        adjust (now, dstadv);
}


bool TimeLord::sunRise (uint8_t *when)
{
    return computeSun (when, true);
}


bool TimeLord::sunSet (uint8_t *when)
{
    return computeSun (when, false);
}


float TimeLord::moonPhase (uint8_t *when)
{
    // The period is 29.530588853 days
    // We compute the number of days since Jan 6, 2000
    // at which time the moon was 'new'
    long d;
    float p;
    
    d = dayNumber (2000 + when[tl_year], when[tl_month] , when[tl_day]) - dayNumber (2000, 1, 6);
    p = d / 29.530588853; // total lunar cycles since 1/1/2000
    d = p;
    p -= d;  // p is now the fractional cycle [0, 1)
    return p;
}


/**
 * Based on US Naval observatory GMST algorithm
 * (http://aa.usno.navy.mil/faq/docs/GAST.php)
 * Adapted for Arduino
 * ---------------------------------------------
 *
 * Since Arduino doesn't provide double precision floating point, we have 
 * modified the algorithm to use (mostly) integer math. 
 * 
 * This implementation will work until the year 2100 with residual error +- 2 seconds.
 * 
 * That translates to +-30 arc-seconds of angular error, which is just
 * about the field of view of a 100x telescope, and well within the field of 
 * view of a 50x scope. 
 */
void TimeLord::sidereal (uint8_t *when, bool local)
{
    uint64_t second, d;
    long minute;

    // We're working in GMT time
    gmt (when);
    
    // Get number of days since our epoch of Jan 1, 2000
    d = dayNumber (2000 + when[tl_year], when[tl_month], when[tl_day]) - dayNumber (2000, 1, 1);

    // Compute calendar seconds since the epoch
    second = d * 86400LL + when[tl_hour] * 3600LL + when[tl_minute] * 60LL + when[tl_second];
    
    // Multiply by ratio of calendar to sidereal time
    second *= 1002737909LL;
    second /= 1000000000LL;
    
    // Add sidereal time at the epoch
    second += 23992LL;
    
    if (local)
    {  // convert from gmt to local
        d = 240.0 * longitude;
        second += d;
    }

    // Constrain to 1 calendar day
    second %= 86400LL;
    
    // Update the tl_time array
    minute = second / 60LL;
    d = minute * 60LL;

    when[tl_second] = second - d;
    when[tl_hour] = 0;
    when[tl_minute] = 0;
    
    adjust (when, minute);    
}


uint8_t TimeLord::_season (uint8_t *when)
{
    if (when[tl_month] < 3)
        return 0;  // winter

    if (when[tl_month] == 3)
    {
        if (when[tl_day] < 22)
            return 0;
        return 1;  // spring
    }
    
    if (when[tl_month] < 6)
        return 1;  // spring

    if (when[tl_month] == 6)
    {
        if(when[tl_day] < 21)
            return 1;
        return 2;  // summer
    }
    
    if (when[tl_month] < 9)
        return 2;  // summer

    if (when[tl_month] == 9)
    {
        if(when[tl_day] < 22)
            return 2;
        return 3;  // fall
    }
    
    if (when[tl_month] < 12)
        return 3;  // summer

    if (when[tl_day] < 21)
        return 3;
    
    return 0;  // winter
}


uint8_t TimeLord::season (uint8_t *when)
{
    uint8_t result;

    result = _season (when);
    if (latitude < 0.f)
        result = (result + 2) % 4;

    return result;
}


uint8_t TimeLord::dayOfWeek (uint8_t *when)
{
    int year;
    uint8_t  month, day;

    year = 2000 + when[tl_year];
    month = when[tl_month];
    day = when[tl_day];
    
    if (month < 3)
    {
        month += 12;
        year--;
    }

    day = ((13 * month + 3) / 5 + day + year + year / 4 - year / 100 + year / 400) % 7;
    day = (day + 1) % 7;
    return day + 1;
}


uint8_t TimeLord::lengthOfMonth (uint8_t *when)
{
    uint8_t odd, month;
    int yr;
    
    yr = 2000 + when[tl_year];
    month = when[tl_month];
    
    if (month == 2)
    {
        if (isLeapYear (yr))
            return 29;
        return 28;
    }

    odd = (month & 1) == 1;
    
    if (month > 7)
        odd = !odd;
    
    if (odd)
        return 31;
    
    return 30;
}


bool TimeLord::isLeapYear (int yr)
{
    return ((yr % 4 == 0 && yr % 100 != 0) || yr % 400 == 0);
}


bool TimeLord::inDst (uint8_t *p)
{
    // Input is assumed to be standard time
    char nSundays, prevSunday, weekday;

    if (p[tl_month] < dstm1 || p[tl_month] > dstm2)
        return false;
    
    if (p[tl_month] > dstm1 && p[tl_month] < dstm2)
        return true;
    
    // If we get here, we are in either the start or end month
    
    // How many sundays so far this month?
    weekday = dayOfWeek (p);
    nSundays = 0;
    prevSunday = p[tl_day] - weekday + 1;
    
    if (prevSunday > 0)
    {
        nSundays = prevSunday / 7;
        nSundays++;
    }
    
    if (p[tl_month] == dstm1)
    {
        if (nSundays < dstw1)
            return false;
        if (nSundays > dstw1)
            return true;
        if (weekday > 1)
            return true;
        if (p[tl_hour] > 1)
            return true;

        return false;
    }
    
    if (nSundays < dstw2)
        return true;
    if (nSundays > dstw2)
        return false;
    if (weekday > 1)
        return false;
    if (p[tl_hour] > 1)
        return false;
    
    return true;
}


// Utility ===============================
// Rather than import yet another library, 
// we define sgn and abs ourselves

uint8_t TimeLord::signum (int n)
{
    return (n < 0) ? -1 : 1;
}


int TimeLord::absolute (int n)
{
    return (n < 0) ? -n : n;
}


void TimeLord::adjust (uint8_t *when, long offset)
{
    long tmp, mod, nxt;
    
    // Offset is in minutes
    tmp = when[tl_minute] + offset;  // minutes
    nxt = tmp / 60;  // hours
    mod = absolute (tmp) % 60;
    mod = mod * signum (tmp) + 60;
    mod %= 60;
    when[tl_minute] = mod;
    
    tmp = nxt + when[tl_hour];
    nxt = tmp / 24;  // days
    mod = absolute (tmp) % 24;
    mod = mod * signum (tmp) + 24;
    mod %= 24;
    when[tl_hour] = mod;
    
    tmp = nxt + when[tl_day];
    mod = lengthOfMonth (when);
    
    if (tmp > mod)
    {
        tmp -= mod;
        when[tl_day] = tmp + 1;
        when[tl_month]++;
    }

    if (tmp < 1)
    {
        when[tl_month]--;
        mod = lengthOfMonth (when);
        when[tl_day] = tmp + mod;
    }
    
    tmp = when[tl_year];
    if (when[tl_month] == 0)
    {
        when[tl_month] = 12;
        tmp--;
    }
    
    if (when[tl_month] > 12)
    {
        when[tl_month] = 1;
        tmp++;
    }

    tmp += 100;
    tmp %= 100;
    when[tl_year] = tmp;  
}


bool TimeLord::computeSun (uint8_t *when, bool rs)
{
    uint8_t month, day;
    float y, decl, eqt, ha, lon, lat, z;
    uint8_t a;
    int doy, minutes;

    month = when[tl_month] - 1;
    day = when[tl_day] - 1;
    lon = -longitude / 57.295779513082322;
    lat = latitude / 57.295779513082322;

    // Approximate hour
    a = 6;
    if (rs) a = 18;

    // Approximate day of year
    y = month * 30.4375 + day + a / 24.0;  // 0... 365

    // Compute fractional year
    y *= 1.718771839885e-02;  // 0... 1

    // Compute equation of time... .43068174
    eqt = 229.18 * (0.000075 + 0.001868 * cos (y) - 0.032077 * sin (y) - \
        0.014615 * cos (y * 2) - 0.040849 * sin (y * 2));

    // Compute solar declination... -0.398272
    decl = 0.006918 - 0.399912 * cos (y) + 0.070257 * sin (y) - 0.006758 * cos (y * 2) + \
        0.000907 * sin (y * 2) - 0.002697 * cos (y * 3) + 0.00148 * sin (y * 3);

    // Compute hour angle
    ha = (cos (1.585340737228125) / (cos (lat) * cos (decl)) - tan (lat) * tan (decl));

    // We're in the (ant)arctic and there is no rise(or set) today!
    if (fabs (ha) > 1.0)
        return false;

    ha = acos (ha);
    if (rs == false)
        ha = -ha;

    // Compute minutes from midnight
    minutes = 720 + 4 * (lon - ha) * 57.295779513082322 - eqt;

    // Convert from UTC back to our timezone
    minutes += timezone;

    // Adjust the time array by minutes
    when[tl_hour] = 0;
    when[tl_minute] = 0;
    when[tl_second] = 0;
    adjust (when, minutes);
    return true;
}


long TimeLord::dayNumber (uint16_t y, uint8_t m, uint8_t d)
{
    m = (m + 9) % 12;
    y -= m / 10;
    return 365 * y + y / 4 - y / 100 + y / 400 + (m * 306 + 5) / 10 + d - 1;
}
