/**
 * @file
 * VuoTime implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <xlocale.h>
#include <langinfo.h>

#include <CoreFoundation/CoreFoundation.h>

#include "type.h"
#include "VuoRoundingMethod.h"
#include "VuoTime.h"
#include "VuoList_VuoTime.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Date-Time",
					  "description" : "A date and time.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoText",
						  "VuoTimeUnit",
						  "VuoList_VuoTime"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{42.0}
 */
VuoTime VuoTime_makeFromJson(json_object *js)
{
	return json_object_get_double(js);
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoTime_getJson(const VuoTime value)
{
	return json_object_new_double(value);
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoTime_getSummary(const VuoTime value)
{
	VuoInteger year;
	VuoInteger month;
	VuoInteger dayOfMonth;
	VuoInteger hour;
	VuoInteger minute;
	VuoReal    second;
	bool ret = VuoTime_getComponents(value, &year, NULL, &month, &dayOfMonth, NULL, NULL, &hour, &minute, &second);
	if (ret)
		return VuoText_format("%04lld-%02lld-%02lld %02lld:%02lld:%05.02f", year, month, dayOfMonth, hour, minute, second);
	else
		return strdup("(unknown)");
}

/**
 * Returns true if the two values are equal.
 */
bool VuoTime_areEqual(const VuoTime valueA, const VuoTime valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoTime_isLessThan(const VuoTime valueA, const VuoTime valueB)
{
	return valueA < valueB;
}

/**
 * Checks if the dates and times are all within a certain distance of each other (within 1 hour, within 8 hours, within 1 year, …).
 */
bool VuoTime_areEqualWithinTolerance(VuoList_VuoTime times, VuoReal tolerance, VuoTimeUnit toleranceUnit)
{
	VuoTime minimumTime = INFINITY;
	int timeCount = VuoListGetCount_VuoTime(times);
	for (int i = 1; i <= timeCount; ++i)
	{
		VuoTime t = VuoListGetValue_VuoTime(times, i);
		if (isnan(t))
			return false;
		if (t < minimumTime)
			minimumTime = t;
	}

	VuoTime toleranceInSeconds = tolerance * VuoTimeUnit_getSeconds(toleranceUnit);
	for (int i = 1; i <= timeCount; ++i)
	{
		VuoTime t = VuoListGetValue_VuoTime(times, i);
		if (fabs(t - minimumTime) > toleranceInSeconds)
			return false;
	}

	return true;
}

/**
 * Removes the date component, so the time is relative to 2001.01.01.
 */
VuoTime VuoTime_removeDate(const VuoTime time)
{
	return fmod(time, 86400);
}

/**
 * Checks if the times are all within a certain distance of each other (within 1 minute, within 8 hours, …).
 *
 * Ignores the date components.
 *
 * Times are considered equal within tolerance if they span midnight.
 * For example, 23:00 and 01:00 are equal within a tolerance of 3 hours.
 */
bool VuoTime_areTimesOfDayEqualWithinTolerance(VuoList_VuoTime times, VuoReal tolerance, VuoTimeUnit toleranceUnit)
{
	VuoTime minimumTime = INFINITY;
	int timeCount = VuoListGetCount_VuoTime(times);
	for (int i = 1; i <= timeCount; ++i)
	{
		VuoTime t = VuoTime_removeDate(VuoListGetValue_VuoTime(times, i));
		if (isnan(t))
			return false;
		if (t < minimumTime)
			minimumTime = t;
	}

	VuoTime toleranceInSeconds = tolerance * VuoTimeUnit_getSeconds(toleranceUnit);
	for (int i = 1; i <= timeCount; ++i)
	{
		VuoTime t = VuoTime_removeDate(VuoListGetValue_VuoTime(times, i));
		if (fabs(t         - minimumTime) > toleranceInSeconds
		 && fabs(t - 86400 - minimumTime) > toleranceInSeconds)
				return false;
	}

	return true;
}

/**
 * Checks if time A is before B, ignoring the date component.
 *
 * `startOfDay` specifies the breakpoint.
 * For example, if `startOfDay` is 04:00, time 23:00 is considered less than 03:00.
 */
bool VuoTime_isTimeOfDayLessThan(const VuoTime valueA, const VuoTime valueB, const VuoTime startOfDay)
{
	VuoTime timeOnlyA     = VuoTime_removeDate(valueA);
	VuoTime timeOnlyB     = VuoTime_removeDate(valueB);
	VuoTime timeOnlyStart = VuoTime_removeDate(startOfDay);

	timeOnlyA = fmod(86400 + timeOnlyA - timeOnlyStart, 86400);
	timeOnlyB = fmod(86400 + timeOnlyB - timeOnlyStart, 86400);

	return timeOnlyA < timeOnlyB;
}

/// The difference between the Unix epoch and the Mac (and Vuo) epoch.
const int VuoUnixTimeOffset = (31 /* 2001-1970 */ * 365 + 8 /* leap days between 1970 and 2001 */) * 24 * 60 * 60;

/**
 * Returns the current calendar date and time (seconds since 2001.01.01 @ 00:00:00 UTC).
 *
 * This function always uses UTC; it does not concern itself with time zones or Daylight Saving Time.
 */
VuoTime VuoTime_getCurrent(void)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return (now.tv_sec - VuoUnixTimeOffset) + now.tv_usec / 1000000.;
}

/**
 * Returns true if the specified year is a Gregorian leap year.
 */
static bool VuoTime_isLeapYear(VuoInteger year)
{
	return (year % 4 == 0)
		&& !((year % 100 == 0)
		  && (year % 400 != 0));
}

/**
 * Creates a date-time from component values.
 *
 * See @ref VuoTime_getComponents for parameter definitions.
 */
VuoTime VuoTime_make(VuoInteger year, VuoInteger month, VuoInteger dayOfMonth, VuoInteger hour, VuoInteger minute, VuoReal second)
{
	struct tm tm;
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = dayOfMonth;
	tm.tm_hour = hour;
	tm.tm_min = minute;
	tm.tm_sec = 0;
	tm.tm_isdst = -1;	// autodetect
	time_t timeInt = mktime(&tm);
	if (timeInt == -1)
	{
		// mktime() failed, so let's try to make it from scratch.

		// Find the number of leaps between the specified year and 1970 (UNIX epoch).
		int leaps = 0;
		for (int y = year; y < 1970; (year<1970 ? ++y : --y))
			if (VuoTime_isLeapYear(y))
				++leaps;

		// For non-leap years.
		char daysPerMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

		int dayOfYear = 0;
		for (int m = 0; m < month-1; ++m)
			dayOfYear += daysPerMonth[m];
		dayOfYear += dayOfMonth;

		if (VuoTime_isLeapYear(year)
		 && dayOfYear > 31 + 28)
			--leaps;

		timeInt = (
					(year - 1970)*365
					+ dayOfYear - 1
					- leaps
				  ) * 60 * 60 * 24
				+ hour * 60 * 60
				+ minute * 60;

		time_t t = time(NULL);
		struct tm lt = {0};
		localtime_r(&t, &lt);
		timeInt -= lt.tm_gmtoff;
	}

	return (timeInt - VuoUnixTimeOffset) + second;
}

/**
 * Creates a date-time from an [RFC 822 date-time string](https://www.ietf.org/rfc/rfc0822.txt).
 *
 * @eg{Fri, 16 Oct 2015 06:42:34 -0400}
 */
VuoTime VuoTime_makeFromRFC822(const char *rfc822)
{
	if (!rfc822 || rfc822[0] == 0)
		return NAN;

	struct tm tm;
	// Initialize tm, since strptime adds to (rather than replacing) tm's initial values (!!).
	// "If time relative to today is desired, initialize the tm structure with today's date before passing it to strptime()."
	bzero(&tm, sizeof(struct tm));

	char *ret = strptime(rfc822, "%a, %d %b %Y %T %z", &tm);
	if (!ret)
	{
		// Parsing failed; try a backup format seen in https://www.nasa.gov/rss/dyn/lg_image_of_the_day.rss
		// `Tue, 12 Jan 2016 11:33 EST`
		ret = strptime(rfc822, "%a, %d %b %Y %R %Z", &tm);
		if (!ret)
			return NAN;
	}

	time_t timeInt = mktime(&tm);
	return timeInt - VuoUnixTimeOffset;
}

/**
 * For a given `time` (in UTC), converts it to the current local timezone, and splits it into its human-readable parts.
 *
 * It's OK to pass NULL for any of the output pointers.
 *
 * @param time The time to dissect.
 * @param year Millennium + Century + Decade + Year (e.g., 1970)
 * @param dayOfYear 1 to 365 or 366
 * @param month 1 to 12
 * @param dayOfMonth 1 to 28, 30, or 31
 * @param week 1 to 52 or 53 (ISO 8601:1988 week number)
 * @param dayOfWeek 0=Sunday, 1=Monday, …, 6=Saturday
 * @param hour 0 to 23
 * @param minute 0 to 59
 * @param second Includes fractional seconds.
 * @return True if the time could be converted.  False for non-times, such as NaN.
 */
bool VuoTime_getComponents(VuoTime time, VuoInteger *year, VuoInteger *dayOfYear, VuoInteger *month, VuoInteger *dayOfMonth, VuoInteger *week, VuoWeekday *dayOfWeek, VuoInteger *hour, VuoInteger *minute, VuoReal *second)
{
	time_t unixtime = time + VuoUnixTimeOffset;
	struct tm *tm = localtime(&unixtime);
	if (!tm)
		return false;
	if (year)
		*year = tm->tm_year + 1900;
	if (dayOfYear)
		*dayOfYear = tm->tm_yday + 1;
	if (month)
		*month = tm->tm_mon + 1;
	if (dayOfMonth)
		*dayOfMonth = tm->tm_mday;
	if (week)
	{
		char s[3];
		strftime(s, 3, "%V", tm);
		*week = atoi(s);
	}
	if (dayOfWeek)
		*dayOfWeek = tm->tm_wday;
	if (hour)
		*hour = tm->tm_hour;
	if (minute)
		*minute = tm->tm_min;
	if (second)
	{
		if (time >= 0)
			*second = fmod(time, 60);
		else
			*second = fmod(60 + fmod(time, 60), 60);
	}

	return true;
}

/**
 * Rounds to a nearby minute, hour, etc.
 */
VuoTime VuoTime_round(const VuoTime value, const VuoTimeUnit unit, const int roundingMethod)
{
	VuoInteger year;
	VuoInteger month;
	VuoInteger dayOfMonth;
	VuoWeekday dayOfWeek;
	VuoInteger hour;
	VuoInteger minute;
	VuoReal    second;
	bool ret = VuoTime_getComponents(value, &year, NULL, &month, &dayOfMonth, NULL, &dayOfWeek, &hour, &minute, &second);
	if (!ret)
		return nan(NULL);

	if (unit == VuoTimeUnit_Millennium)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(1000*(year/1000) + (year%1000 > 500 ? 1000 : 0), 1, 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(1000*(year/1000), 1, 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(1000*(year/1000) + (year%1000 + month + dayOfMonth + hour + minute + second > 0 ? 1000 : 0), 1, 1, 0, 0, 0);
	}
	else if (unit == VuoTimeUnit_Century)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(100*(year/100) + (year%100 > 50 ? 100 : 0), 1, 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(100*(year/100), 1, 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(100*(year/100) + (year%100 + month + dayOfMonth + hour + minute + second > 0 ? 100 : 0), 1, 1, 0, 0, 0);
	}
	else if (unit == VuoTimeUnit_Decade)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(10*(year/10) + (year%10 > 5 ? 10 : 0), 1, 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(10*(year/10), 1, 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(10*(year/10) + (year%10 + month + dayOfMonth + hour + minute + second > 0 ? 10 : 0), 1, 1, 0, 0, 0);
	}
	else if (unit == VuoTimeUnit_Year)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(year + (month > 6 ? 1 : 0), 1, 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(year, 1, 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(year + (month + dayOfMonth + hour + minute + second > 0 ? 1 : 0), 1, 1, 0, 0, 0);
	}
	else if (unit == VuoTimeUnit_Quarter)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(year, 3*((month-1)/3) + 1 + ((month-1)%3 > 1 ? 3 : 0), 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(year, 3*((month-1)/3) + 1, 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(year, 3*((month-1)/3) + 1 + ((month-1)%3 + dayOfMonth + hour + minute + second > 0 ? 3 : 0), 1, 0, 0, 0);
	}
	else if (unit == VuoTimeUnit_Month)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(year, month + (dayOfMonth > 15 ? 1 : 0), 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(year, month, 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(year, month + (dayOfMonth + hour + minute + second > 0 ? 1 : 0), 1, 0, 0, 0);
	}
	else if (unit == VuoTimeUnit_WeekSunday)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(year, month, dayOfMonth + (dayOfWeek > VuoWeekday_Wednesday ? 7-dayOfWeek : -dayOfWeek), 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(year, month, dayOfMonth - dayOfWeek, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(year, month, dayOfMonth + (dayOfWeek + hour + minute + second > 0 ? 7-dayOfWeek : 0), 0, 0, 0);
	}
	else if (unit == VuoTimeUnit_WeekMonday)
	{
		if (dayOfWeek == VuoWeekday_Sunday)
			dayOfWeek += 7;

		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(year, month, dayOfMonth + (dayOfWeek > VuoWeekday_Thursday ? 8-dayOfWeek : 1-dayOfWeek), 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(year, month, dayOfMonth - dayOfWeek + 1, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(year, month, dayOfMonth + (dayOfWeek-1 + hour + minute + second > 0 ? 8-dayOfWeek : 0), 0, 0, 0);
	}
	else if (unit == VuoTimeUnit_Day)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(year, month, dayOfMonth + (hour > 12 ? 1 : 0), 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(year, month, dayOfMonth, 0, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(year, month, dayOfMonth + (hour + minute + second > 0 ? 1 : 0), 0, 0, 0);
	}
	else if (unit == VuoTimeUnit_Hour)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(year, month, dayOfMonth, hour + (minute > 30 ? 1 : 0), 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(year, month, dayOfMonth, hour, 0, 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(year, month, dayOfMonth, hour + (minute + second > 0 ? 1 : 0), 0, 0);
	}
	else if (unit == VuoTimeUnit_HalfHour)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(year, month, dayOfMonth, hour, 30*(minute/30) + (minute%30 > 15 ? 30 : 0), 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(year, month, dayOfMonth, hour, 30*(minute/30), 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(year, month, dayOfMonth, hour, 30*(minute/30) + (minute%30 + second > 0 ? 30 : 0), 0);
	}
	else if (unit == VuoTimeUnit_QuarterHour)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(year, month, dayOfMonth, hour, 15*(minute/15) + (minute%15 > 7 ? 15 : 0), 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(year, month, dayOfMonth, hour, 15*(minute/15), 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(year, month, dayOfMonth, hour, 15*(minute/15) + (minute%15 + second > 0 ? 15 : 0), 0);
	}
	else if (unit == VuoTimeUnit_Minute)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return VuoTime_make(year, month, dayOfMonth, hour, minute + (second > 30 ? 1 : 0), 0);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return VuoTime_make(year, month, dayOfMonth, hour, minute, 0);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return VuoTime_make(year, month, dayOfMonth, hour, minute + (second > 0 ? 1 : 0), 0);
	}
	else if (unit == VuoTimeUnit_Second)
	{
		if (roundingMethod == VuoRoundingMethod_Nearest)
			return lround(value);
		else if (roundingMethod == VuoRoundingMethod_Down)
			return floor(value);
		else if (roundingMethod == VuoRoundingMethod_Up)
			return ceil(value);
	}

	return value;
}

/**
 * Outputs text containing a date and/or time, in the system's current locale.
 *
 * Uses the current local timezone (except VuoTimeFormat_DateTimeSortable, which is always UTC).
 *
 * Fractional seconds are rounded down.
 */
VuoText VuoTime_format(const VuoTime time, const VuoTimeFormat format)
{
	CFLocaleRef localeCF = CFLocaleCopyCurrent();
	VuoText localeID = VuoText_makeFromCFString(CFLocaleGetIdentifier(localeCF));
	VuoRetain(localeID);
	CFRelease(localeCF);

	VuoText formattedTime = VuoTime_formatWithLocale(time, format, localeID);

	VuoRelease(localeID);

	return formattedTime;
}

/**
 * Outputs text containing a date and/or time.
 *
 * Uses the current local timezone (except VuoTimeFormat_DateTimeSortable, which is always UTC).
 *
 * Fractional seconds are rounded down.
 */
VuoText VuoTime_formatWithLocale(const VuoTime time, const VuoTimeFormat format, const VuoText localeIdentifier)
{
	locale_t locale = newlocale(LC_ALL_MASK, localeIdentifier, NULL);
	static dispatch_once_t loggedLocale = 0;
	dispatch_once(&loggedLocale, ^{
		VUserLog("OS X Locale: %s", localeIdentifier);
		VUserLog("   C Locale: %s", querylocale(LC_ALL_MASK, locale));
	});

	const char *dateFormatString   = nl_langinfo_l(D_FMT,      locale);
	const char *time12FormatString = nl_langinfo_l(T_FMT_AMPM, locale);
	const char *time24FormatString = nl_langinfo_l(T_FMT,      locale);

	VuoText output = NULL;
	char *formatString = NULL;
	if (format == VuoTimeFormat_DateTimeShort12)
		formatString = VuoText_format("%s %s", dateFormatString, time12FormatString);
	else if (format == VuoTimeFormat_DateTimeShort24)
		formatString = VuoText_format("%s %s", dateFormatString, time24FormatString);
	else if (format == VuoTimeFormat_DateTimeMedium12)
		formatString = VuoText_format("%%b %%e, %%Y %s", time12FormatString);
	else if (format == VuoTimeFormat_DateTimeMedium24)
		formatString = VuoText_format("%%b %%e, %%Y %s", time24FormatString);
	else if (format == VuoTimeFormat_DateTimeLong12)
		formatString = VuoText_format("%%A, %%B %%e, %%Y %s", time12FormatString);
	else if (format == VuoTimeFormat_DateTimeLong24)
		formatString = VuoText_format("%%A, %%B %%e, %%Y %s", time24FormatString);
	else if (format == VuoTimeFormat_DateTimeSortable)
		formatString = strdup("%Y-%m-%d %H:%M:%SZ");
	else if (format == VuoTimeFormat_DateShort)
		formatString = strdup(dateFormatString);
	else if (format == VuoTimeFormat_DateMedium)
		formatString = strdup("%b %e, %Y");
	else if (format == VuoTimeFormat_DateLong)
		formatString = strdup("%A, %B %e, %Y");
	else if (format == VuoTimeFormat_Time12)
		formatString = strdup(time12FormatString);
	else if (format == VuoTimeFormat_Time24)
		formatString = strdup(time24FormatString);
	else
		goto done;

	time_t t = time + VuoUnixTimeOffset;
	struct tm *tm;
	if (format == VuoTimeFormat_DateTimeSortable)
		tm = gmtime(&t);
	else
		tm = localtime(&t);
	if (!tm)
		goto done;

	char formattedTime[1024];
	strftime_l(formattedTime, 1024, formatString, tm, locale);

	output = VuoText_make(formattedTime);

done:
	free(formatString);
	freelocale(locale);

	return output;
}
