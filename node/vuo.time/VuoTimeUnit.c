/**
 * @file
 * VuoTimeUnit implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoTimeUnit.h"
#include "VuoList_VuoTimeUnit.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Time Unit",
					  "description" : "A delineation of time.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  "VuoList_VuoTimeUnit"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "quarter"
 * }
 */
VuoTimeUnit VuoTimeUnit_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoTimeUnit value = VuoTimeUnit_Millennium;

	if (strcmp(valueAsString, "century") == 0)
		value = VuoTimeUnit_Century;
	else if (strcmp(valueAsString, "decade") == 0)
		value = VuoTimeUnit_Decade;
	else if (strcmp(valueAsString, "year") == 0)
		value = VuoTimeUnit_Year;
	else if (strcmp(valueAsString, "quarter") == 0)
		value = VuoTimeUnit_Quarter;
	else if (strcmp(valueAsString, "month") == 0)
		value = VuoTimeUnit_Month;
	else if (strcmp(valueAsString, "week-sunday") == 0)
		value = VuoTimeUnit_WeekSunday;
	else if (strcmp(valueAsString, "week-monday") == 0)
		value = VuoTimeUnit_WeekMonday;
	else if (strcmp(valueAsString, "day") == 0)
		value = VuoTimeUnit_Day;
	else if (strcmp(valueAsString, "hour") == 0)
		value = VuoTimeUnit_Hour;
	else if (strcmp(valueAsString, "half-hour") == 0)
		value = VuoTimeUnit_HalfHour;
	else if (strcmp(valueAsString, "quarter-hour") == 0)
		value = VuoTimeUnit_QuarterHour;
	else if (strcmp(valueAsString, "minute") == 0)
		value = VuoTimeUnit_Minute;
	else if (strcmp(valueAsString, "second") == 0)
		value = VuoTimeUnit_Second;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoTimeUnit_getJson(const VuoTimeUnit value)
{
	char *valueAsString = "millennium";

	if (value == VuoTimeUnit_Century)
		valueAsString = "century";
	else if (value == VuoTimeUnit_Decade)
		valueAsString = "decade";
	else if (value == VuoTimeUnit_Year)
		valueAsString = "year";
	else if (value == VuoTimeUnit_Quarter)
		valueAsString = "quarter";
	else if (value == VuoTimeUnit_Month)
		valueAsString = "month";
	else if (value == VuoTimeUnit_WeekSunday)
		valueAsString = "week-sunday";
	else if (value == VuoTimeUnit_WeekMonday)
		valueAsString = "week-monday";
	else if (value == VuoTimeUnit_Day)
		valueAsString = "day";
	else if (value == VuoTimeUnit_Hour)
		valueAsString = "hour";
	else if (value == VuoTimeUnit_HalfHour)
		valueAsString = "half-hour";
	else if (value == VuoTimeUnit_QuarterHour)
		valueAsString = "quarter-hour";
	else if (value == VuoTimeUnit_Minute)
		valueAsString = "minute";
	else if (value == VuoTimeUnit_Second)
		valueAsString = "second";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoTimeUnit VuoTimeUnit_getAllowedValues(void)
{
	VuoList_VuoTimeUnit l = VuoListCreate_VuoTimeUnit();
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_Millennium);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_Century);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_Decade);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_Year);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_Quarter);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_Month);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_WeekSunday);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_WeekMonday);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_Day);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_Hour);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_HalfHour);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_QuarterHour);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_Minute);
	VuoListAppendValue_VuoTimeUnit(l, VuoTimeUnit_Second);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoTimeUnit_getSummary(const VuoTimeUnit value)
{
	char *valueAsString = "Millennium";

	if (value == VuoTimeUnit_Century)
		valueAsString = "Century";
	else if (value == VuoTimeUnit_Decade)
		valueAsString = "Decade";
	else if (value == VuoTimeUnit_Year)
		valueAsString = "Year";
	else if (value == VuoTimeUnit_Quarter)
		valueAsString = "Quarter";
	else if (value == VuoTimeUnit_Month)
		valueAsString = "Month";
	else if (value == VuoTimeUnit_WeekSunday)
		valueAsString = "Week (Sunday–Saturday)";
	else if (value == VuoTimeUnit_WeekMonday)
		valueAsString = "Week (Monday–Sunday)";
	else if (value == VuoTimeUnit_Day)
		valueAsString = "Day";
	else if (value == VuoTimeUnit_Hour)
		valueAsString = "Hour";
	else if (value == VuoTimeUnit_HalfHour)
		valueAsString = "Half Hour";
	else if (value == VuoTimeUnit_QuarterHour)
		valueAsString = "Quarter Hour";
	else if (value == VuoTimeUnit_Minute)
		valueAsString = "Minute";
	else if (value == VuoTimeUnit_Second)
		valueAsString = "Second";

	return strdup(valueAsString);
}

/**
 * Returns the number of seconds the specified time unit is worth.
 *
 * - Years are 365 days (leap years are ignored)
 * - Quarters are 121 days
 * - Months are 30 days (28-, 29-, and 31-day months are ignored)
 */
VuoInteger VuoTimeUnit_getSeconds(const VuoTimeUnit value)
{
	if (value == VuoTimeUnit_Millennium)
		return 60ULL * 60 * 24 * 365 * 1000;
	else if (value == VuoTimeUnit_Century)
		return 60ULL * 60 * 24 * 365 * 100;
	else if (value == VuoTimeUnit_Decade)
		return 60ULL * 60 * 24 * 365 * 10;
	else if (value == VuoTimeUnit_Year)
		return 60ULL * 60 * 24 * 365;
	else if (value == VuoTimeUnit_Quarter)
		return 60ULL * 60 * 24 * 121;
	else if (value == VuoTimeUnit_Month)
		return 60ULL * 60 * 24 * 30;
	else if (value == VuoTimeUnit_WeekSunday)
		return 60ULL * 60 * 24 * 7;
	else if (value == VuoTimeUnit_WeekMonday)
		return 60ULL * 60 * 24 * 7;
	else if (value == VuoTimeUnit_Day)
		return 60ULL * 60 * 24;
	else if (value == VuoTimeUnit_Hour)
		return 60ULL * 60;
	else if (value == VuoTimeUnit_HalfHour)
		return 60ULL * 30;
	else if (value == VuoTimeUnit_QuarterHour)
		return 60ULL * 15;
	else if (value == VuoTimeUnit_Minute)
		return 60ULL;
	else //if (value == VuoTimeUnit_Second)
		return 1ULL;
}

/**
 * Returns true if the two values are equal.
 */
bool VuoTimeUnit_areEqual(const VuoTimeUnit valueA, const VuoTimeUnit valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoTimeUnit_isLessThan(const VuoTimeUnit valueA, const VuoTimeUnit valueB)
{
	return valueA < valueB;
}

