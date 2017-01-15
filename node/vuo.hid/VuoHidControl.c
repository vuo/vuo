/**
 * @file
 * VuoHidControl implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoHidControl.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "HID Control",
					  "description" : "Information about a control on a USB HID device.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoInteger",
						  "VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "name": "System Menu Left",
 *     "value": 0,
 *     "min": 0,
 *     "max": 1
 *   }
 * }
 */
VuoHidControl VuoHidControl_makeFromJson(json_object *js)
{
	VuoHidControl value = {NULL, 0, 0, 0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_makeFromJson(o);
	if (json_object_object_get_ex(js, "value", &o))
		value.value = VuoInteger_makeFromJson(o);
	if (json_object_object_get_ex(js, "min", &o))
		value.min = VuoInteger_makeFromJson(o);
	if (json_object_object_get_ex(js, "max", &o))
		value.max = VuoInteger_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoHidControl_getJson(const VuoHidControl value)
{
	json_object *js = json_object_new_object();

	if (value.name)
	{
		json_object *o = VuoText_getJson(value.name);
		json_object_object_add(js, "name", o);
	}

	if (value.value)
	{
		json_object *o = VuoInteger_getJson(value.value);
		json_object_object_add(js, "value", o);
	}

	if (value.min)
	{
		json_object *o = VuoInteger_getJson(value.min);
		json_object_object_add(js, "min", o);
	}

	if (value.max)
	{
		json_object *o = VuoInteger_getJson(value.max);
		json_object_object_add(js, "max", o);
	}

	return js;
}

/**
 * Returns true if the two values are equal.
 */
bool VuoHidControl_areEqual(const VuoHidControl valueA, const VuoHidControl valueB)
{
	if (!VuoText_areEqual(valueA.name, valueB.name))
		return false;

	if (!VuoInteger_areEqual(valueA.value, valueB.value))
		return false;

	if (!VuoInteger_areEqual(valueA.min, valueB.min))
		return false;

	if (!VuoInteger_areEqual(valueA.max, valueB.max))
		return false;

	return true;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoHidControl_isLessThan(const VuoHidControl a, const VuoHidControl b)
{
	if (VuoText_isLessThan(a.name, b.name)) return true;
	if (VuoText_isLessThan(b.name, a.name)) return false;

	if (a.value < b.value) return true;
	if (a.value > b.value) return false;

	if (a.min < b.min) return true;
	if (a.min > b.min) return false;

	if (a.max < b.max) return true;
//	if (a.max > b.max) return false;

	return false;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoHidControl_getSummary(const VuoHidControl value)
{
	if (value.name)
		return VuoText_format("%s = %lld", value.name, value.value);
	else
		return strdup("(no control)");
}
