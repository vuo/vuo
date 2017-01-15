/**
 * @file
 * VuoWeekday implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoWeekday.h"
#include "VuoList_VuoWeekday.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Weekday",
					  "description" : "A day of the week",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  "VuoList_VuoWeekday"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "monday"
 * }
 */
VuoWeekday VuoWeekday_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoWeekday value = VuoWeekday_Sunday;

	if (strcmp(valueAsString, "monday") == 0)
		value = VuoWeekday_Monday;
	else if (strcmp(valueAsString, "tuesday") == 0)
		value = VuoWeekday_Tuesday;
	else if (strcmp(valueAsString, "wednesday") == 0)
		value = VuoWeekday_Wednesday;
	else if (strcmp(valueAsString, "thursday") == 0)
		value = VuoWeekday_Thursday;
	else if (strcmp(valueAsString, "friday") == 0)
		value = VuoWeekday_Friday;
	else if (strcmp(valueAsString, "saturday") == 0)
		value = VuoWeekday_Saturday;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoWeekday_getJson(const VuoWeekday value)
{
	char *valueAsString = "sunday";

	if (value == VuoWeekday_Monday)
		valueAsString = "monday";
	else if (value == VuoWeekday_Tuesday)
		valueAsString = "tuesday";
	else if (value == VuoWeekday_Wednesday)
		valueAsString = "wednesday";
	else if (value == VuoWeekday_Thursday)
		valueAsString = "thursday";
	else if (value == VuoWeekday_Friday)
		valueAsString = "friday";
	else if (value == VuoWeekday_Saturday)
		valueAsString = "saturday";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoWeekday VuoWeekday_getAllowedValues(void)
{
	VuoList_VuoWeekday l = VuoListCreate_VuoWeekday();
	VuoListAppendValue_VuoWeekday(l, VuoWeekday_Sunday);
	VuoListAppendValue_VuoWeekday(l, VuoWeekday_Monday);
	VuoListAppendValue_VuoWeekday(l, VuoWeekday_Tuesday);
	VuoListAppendValue_VuoWeekday(l, VuoWeekday_Wednesday);
	VuoListAppendValue_VuoWeekday(l, VuoWeekday_Thursday);
	VuoListAppendValue_VuoWeekday(l, VuoWeekday_Friday);
	VuoListAppendValue_VuoWeekday(l, VuoWeekday_Saturday);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoWeekday_getSummary(const VuoWeekday value)
{
	char *valueAsString = "Sunday";

	if (value == VuoWeekday_Monday)
		valueAsString = "Monday";
	else if (value == VuoWeekday_Tuesday)
		valueAsString = "Tuesday";
	else if (value == VuoWeekday_Wednesday)
		valueAsString = "Wednesday";
	else if (value == VuoWeekday_Thursday)
		valueAsString = "Thursday";
	else if (value == VuoWeekday_Friday)
		valueAsString = "Friday";
	else if (value == VuoWeekday_Saturday)
		valueAsString = "Saturday";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoWeekday_areEqual(const VuoWeekday valueA, const VuoWeekday valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoWeekday_isLessThan(const VuoWeekday valueA, const VuoWeekday valueB)
{
	return valueA < valueB;
}
