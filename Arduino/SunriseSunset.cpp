#include "SunriseSunset.h"
#include "math.h"

/* Helper functions */

// Source credits : https://mariusbancila.ro/blog/2017/08/03/computing-day-of-year-in-c/
unsigned int days_to_month[2][12] =
{
 // non-leap year
 { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
 // leap year
 { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 },
};
bool is_leap(int const year) noexcept
{
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
unsigned int day_of_year(int const year, unsigned int const month, unsigned int const day)
{
  return days_to_month[is_leap(year)][month - 1] + day;
}
/***********************************/



// Returns the UTC time
unsigned int GetDefaultTimezone(DateTime d)
{
	unsigned int timezone = 0;

	return timezone;
}

// Creates a DateTime object from a minutes amount
DateTime GetDateTimeFromMinutes(DateTime d, float minutes)
{
	int hours = minutes / 60;
	float minutes_out = minutes - hours * 60;
	int seconds = (minutes_out - (int)minutes_out) * 60;
	return DateTime(d.year(), d.month(), d.day(), hours,  (int)minutes_out, seconds);
}

// Function that calculates sunrise and sunset time for the given day from lattitude and longitude of a given location
// Lattitude and longitude are in degrees
// DateTime is the date at which we'd like to know sunset and sunrise times
// Timezone is in hours from UTC
SunriseSunsetDates CalculateSunriseSunset(float lattitude, float longitude, DateTime d, float timezone)
{
	// Source :	https://www.esrl.noaa.gov/gmd/grad/solcalc/solareqns.PDF
	unsigned int dayOfYear = day_of_year(d.year(), d.month(), d.day());
	float daysInAYear = 365.0;
	if( is_leap(d.year()) )
	{
		daysInAYear = 366.0;
	}
	float gamma = 2.0 * PI / daysInAYear * (dayOfYear + (12.0 + timezone - 12.0) / 24.0);
	float eqtime = 229.18*(0.000075 + 0.001868*cos(gamma) - 0.032077*sin(gamma) - 0.014615*cos(2*gamma) - 0.040849*sin(2*gamma) );
	float decl = 0.006918 - 0.399912*cos(gamma)+ 0.070257*sin(gamma)-0.006758*cos(2*gamma)+ 0.000907*sin(2*gamma)-0.002697*cos(3*gamma)+ 0.00148*sin(3*gamma);

	float ha = acos( cos(90.833 * DEG_TO_RAD) / ( cos(lattitude * DEG_TO_RAD) * cos(decl) ) - tan(lattitude * DEG_TO_RAD)*tan(decl) ) * RAD_TO_DEG;

	float sunrise= 720.0 - 4.0*(longitude+ha)-eqtime + (timezone * 60.0);
	float sunset= 720.0 - 4.0*(longitude-ha)-eqtime + (timezone * 60.0);

	SunriseSunsetDates dates;
	dates.SunriseDate = GetDateTimeFromMinutes(d, sunrise);
	dates.SunsetDate = GetDateTimeFromMinutes(d, sunset);

	return dates;
}
