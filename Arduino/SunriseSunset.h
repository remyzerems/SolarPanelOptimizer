#ifndef __SUNRISE_SUNSET_H__
#define __SUNRISE_SUNSET_H__

#include "RTClib.h"

typedef struct
{
	DateTime SunriseDate;
	DateTime SunsetDate;
} SunriseSunsetDates;

SunriseSunsetDates CalculateSunriseSunset(float lattitude, float longitude, DateTime d, float timezone);

#endif
