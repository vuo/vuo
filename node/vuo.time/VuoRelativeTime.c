/**
 * @file
 * VuoRelativeTime implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoRelativeTime.h"
#include "VuoList_VuoRelativeTime.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Relative Date-Time",
					  "description" : "An offset from a Date-Time.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoRelativeTime",
						  "VuoList_VuoText",
						  "VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{42.0}
 */
VuoRelativeTime VuoRelativeTime_makeFromJson(json_object *js)
{
	return json_object_get_double(js);
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoRelativeTime_getJson(const VuoRelativeTime value)
{
	return json_object_new_double(value);
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoRelativeTime_getSummary(const VuoRelativeTime value)
{
	const char *after  = " after";
	const char *before = " before";

	if (fabs(value) < 0.00001)
		return strdup("no offset");

	VuoInteger years;
	VuoInteger months;
	VuoInteger days;
	VuoInteger hours;
	VuoInteger minutes;
	VuoReal    seconds;
	VuoRelativeTime_getComponents(value, &years, &months, &days, &hours, &minutes, &seconds);

	VuoList_VuoText components = VuoListCreate_VuoText();
	VuoRetain(components);

	if (years)
	{
		char *s = VuoText_format("%d year%s", abs(years), abs(years) == 1 ? "" : "s");
		VuoListAppendValue_VuoText(components, VuoText_make(s));
		free(s);
	}

	if (months)
	{
		char *s = VuoText_format("%d month%s", abs(months), abs(months) == 1 ? "" : "s");
		VuoListAppendValue_VuoText(components, VuoText_make(s));
		free(s);
	}

	if (days)
	{
		char *s = VuoText_format("%d day%s", abs(days), abs(days) == 1 ? "" : "s");
		VuoListAppendValue_VuoText(components, VuoText_make(s));
		free(s);
	}

	if (hours)
	{
		char *s = VuoText_format("%d hour%s", abs(hours), abs(hours) == 1 ? "" : "s");
		VuoListAppendValue_VuoText(components, VuoText_make(s));
		free(s);
	}

	if (minutes)
	{
		char *s = VuoText_format("%d minute%s", abs(minutes), abs(minutes) == 1 ? "" : "s");
		VuoListAppendValue_VuoText(components, VuoText_make(s));
		free(s);
	}

	if (seconds)
	{
		char *s = VuoText_format("%g second%s", fabs(seconds), fabs(seconds) == 1 ? "" : "s");
		VuoListAppendValue_VuoText(components, VuoText_make(s));
		free(s);
	}

	int componentCount = VuoListGetCount_VuoText(components);
	if (componentCount == 1)
	{
		char *s = VuoText_format("%s%s",
								 VuoListGetValue_VuoText(components, 1),
								 value > 0 ? after : before);
		VuoRelease(components);
		return s;
	}
	else if (componentCount == 2)
	{
		char *s = VuoText_format("%s and %s%s",
								 VuoListGetValue_VuoText(components, 1),
								 VuoListGetValue_VuoText(components, 2),
								 value > 0 ? after : before);
		VuoRelease(components);
		return s;
	}
	else
	{
		VuoText componentsAndSeparators[componentCount*2];
		for (int i = 1; i <= componentCount; ++i)
		{
			componentsAndSeparators[(i-1)*2] = VuoListGetValue_VuoText(components, i);
			if (i < componentCount)
				componentsAndSeparators[(i-1)*2 + 1] = (i == componentCount - 1) ? ", and " : ", ";
		}
		componentsAndSeparators[componentCount*2 - 1] = value > 0 ? after : before;
		VuoText t = VuoText_append(componentsAndSeparators, componentCount*2);
		VuoRetain(t);
		char *s = strdup(t);
		VuoRelease(t);
		VuoRelease(components);
		return s;
	}
}

/**
 * Returns true if the two values are equal.
 */
bool VuoRelativeTime_areEqual(const VuoRelativeTime valueA, const VuoRelativeTime valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoRelativeTime_isLessThan(const VuoRelativeTime valueA, const VuoRelativeTime valueB)
{
	return valueA < valueB;
}

/**
 * Creates a relative date-time from component values.
 *
 * See @ref VuoRelativeTime_getComponents for parameter definitions.
 */
VuoRelativeTime VuoRelativeTime_make(VuoInteger years, VuoInteger months, VuoInteger days, VuoInteger hours, VuoInteger minutes, VuoReal seconds)
{
	return (((years * 365 + months * 30 + days) * 24 + hours) * 60 + minutes) * 60 + seconds;
}

/**
 * Splits a given `relativeTime` into its human-readable parts.
 *
 * It's OK to pass NULL for any of the output pointers.
 *
 * @param relativeTime The offset to dissect.
 * @param years 365 days (leap years are ignored)
 * @param months 30 days (28-, 29-, and 31-day months are ignored)
 * @param days 24 hours
 * @param hours 60 minutes
 * @param minutes 60 seconds
 * @param seconds Includes fractional seconds.
 */
void VuoRelativeTime_getComponents(VuoRelativeTime relativeTime, VuoInteger *years, VuoInteger *months, VuoInteger *days, VuoInteger *hours, VuoInteger *minutes, VuoReal *seconds)
{
	VuoInteger minuteLength = 60;
	VuoInteger hourLength   = minuteLength * 60;
	VuoInteger dayLength    = hourLength   * 24;
	VuoInteger monthLength  = dayLength    * 30;
	VuoInteger yearLength   = dayLength    * 365;

	if (years)
		*years = (VuoInteger)relativeTime / yearLength;
	relativeTime = fmod(relativeTime, yearLength);

	if (months)
		*months = (VuoInteger)relativeTime / monthLength;
	relativeTime = fmod(relativeTime, monthLength);

	if (days)
		*days = (VuoInteger)relativeTime / dayLength;
	relativeTime = fmod(relativeTime, dayLength);

	if (hours)
		*hours = (VuoInteger)relativeTime / hourLength;
	relativeTime = fmod(relativeTime, hourLength);

	if (minutes)
		*minutes = (VuoInteger)relativeTime / minuteLength;

	if (seconds)
		*seconds = fmod(relativeTime, 60);
}
