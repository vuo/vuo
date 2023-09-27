/**
 * @file
 * VuoTimeFormat implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTime.h"
#include "VuoTimeFormat.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Date-Time Format",
					  "description" : "How to format a Date-Time.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoTime",
						  "VuoList_VuoTimeFormat"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "D_FMT T_FMT"
 * }
 */
VuoTimeFormat VuoTimeFormat_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoTimeFormat value = VuoTimeFormat_DateTimeSortable;

	if (strcmp(valueAsString, "datetime-short-24") == 0)
		value = VuoTimeFormat_DateTimeShort24;
	else if (strcmp(valueAsString, "datetime-short-12") == 0)
		value = VuoTimeFormat_DateTimeShort12;
	else if (strcmp(valueAsString, "datetime-medium-12") == 0)
		value = VuoTimeFormat_DateTimeMedium12;
	else if (strcmp(valueAsString, "datetime-medium-24") == 0)
		value = VuoTimeFormat_DateTimeMedium24;
	else if (strcmp(valueAsString, "datetime-long-12") == 0)
		value = VuoTimeFormat_DateTimeLong12;
	else if (strcmp(valueAsString, "datetime-long-24") == 0)
		value = VuoTimeFormat_DateTimeLong24;
	else if (strcmp(valueAsString, "date-short") == 0)
		value = VuoTimeFormat_DateShort;
	else if (strcmp(valueAsString, "date-medium") == 0)
		value = VuoTimeFormat_DateMedium;
	else if (strcmp(valueAsString, "date-long") == 0)
		value = VuoTimeFormat_DateLong;
	else if (strcmp(valueAsString, "time-12") == 0)
		value = VuoTimeFormat_Time12;
	else if (strcmp(valueAsString, "time-24") == 0)
		value = VuoTimeFormat_Time24;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoTimeFormat_getJson(const VuoTimeFormat value)
{
	char *valueAsString = "datetime-sortable";

	if (value == VuoTimeFormat_DateTimeShort12)
		valueAsString = "datetime-short-12";
	else if (value == VuoTimeFormat_DateTimeShort24)
		valueAsString = "datetime-short-24";
	else if (value == VuoTimeFormat_DateTimeMedium12)
		valueAsString = "datetime-medium-12";
	else if (value == VuoTimeFormat_DateTimeMedium24)
		valueAsString = "datetime-medium-24";
	else if (value == VuoTimeFormat_DateTimeLong12)
		valueAsString = "datetime-long-12";
	else if (value == VuoTimeFormat_DateTimeLong24)
		valueAsString = "datetime-long-24";
	else if (value == VuoTimeFormat_DateShort)
		valueAsString = "date-short";
	else if (value == VuoTimeFormat_DateMedium)
		valueAsString = "date-medium";
	else if (value == VuoTimeFormat_DateLong)
		valueAsString = "date-long";
	else if (value == VuoTimeFormat_Time12)
		valueAsString = "time-12";
	else if (value == VuoTimeFormat_Time24)
		valueAsString = "time-24";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoTimeFormat VuoTimeFormat_getAllowedValues(void)
{
	VuoList_VuoTimeFormat l = VuoListCreate_VuoTimeFormat();
	VuoListAppendValue_VuoTimeFormat(l, VuoTimeFormat_DateTimeSortable);
	VuoListAppendValue_VuoTimeFormat(l, VuoTimeFormat_DateTimeShort12);
	VuoListAppendValue_VuoTimeFormat(l, VuoTimeFormat_DateTimeShort24);
	VuoListAppendValue_VuoTimeFormat(l, VuoTimeFormat_DateTimeMedium12);
	VuoListAppendValue_VuoTimeFormat(l, VuoTimeFormat_DateTimeMedium24);
	VuoListAppendValue_VuoTimeFormat(l, VuoTimeFormat_DateTimeLong12);
	VuoListAppendValue_VuoTimeFormat(l, VuoTimeFormat_DateTimeLong24);
	VuoListAppendValue_VuoTimeFormat(l, VuoTimeFormat_DateShort);
	VuoListAppendValue_VuoTimeFormat(l, VuoTimeFormat_DateMedium);
	VuoListAppendValue_VuoTimeFormat(l, VuoTimeFormat_DateLong);
	VuoListAppendValue_VuoTimeFormat(l, VuoTimeFormat_Time12);
	VuoListAppendValue_VuoTimeFormat(l, VuoTimeFormat_Time24);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoTimeFormat_getSummary(const VuoTimeFormat value)
{
	VuoText formatted = VuoTime_format(VuoTime_getCurrent(), value);
	VuoLocal(formatted);
	if (value == VuoTimeFormat_DateTimeSortable)
		return VuoText_format("Date & Time — Sortable, UTC (%s)", formatted);
	else if (value == VuoTimeFormat_DateTimeShort12)
		return VuoText_format("Date & Time — Short — 12-hour (%s)", formatted);
	else if (value == VuoTimeFormat_DateTimeShort24)
		return VuoText_format("Date & Time — Short — 24-hour (%s)", formatted);
	else if (value == VuoTimeFormat_DateTimeMedium12)
		return VuoText_format("Date & Time — Medium 12-hour (%s)", formatted);
	else if (value == VuoTimeFormat_DateTimeMedium24)
		return VuoText_format("Date & Time — Medium 24-hour (%s)", formatted);
	else if (value == VuoTimeFormat_DateTimeLong12)
		return VuoText_format("Date & Time — Long 12-hour (%s)", formatted);
	else if (value == VuoTimeFormat_DateTimeLong24)
		return VuoText_format("Date & Time — Long 24-hour (%s)", formatted);
	else if (value == VuoTimeFormat_DateShort)
		return VuoText_format("Date — Short (%s)", formatted);
	else if (value == VuoTimeFormat_DateMedium)
		return VuoText_format("Date — Medium (%s)", formatted);
	else if (value == VuoTimeFormat_DateLong)
		return VuoText_format("Date — Long (%s)", formatted);
	else if (value == VuoTimeFormat_Time12)
		return VuoText_format("Time — 12-hour (%s)", formatted);
	else if (value == VuoTimeFormat_Time24)
		return VuoText_format("Time — 24-hour (%s)", formatted);
	else
		return strdup(formatted);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoTimeFormat_areEqual(const VuoTimeFormat valueA, const VuoTimeFormat valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoTimeFormat_isLessThan(const VuoTimeFormat valueA, const VuoTimeFormat valueB)
{
	return valueA < valueB;
}
