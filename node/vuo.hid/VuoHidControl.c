/**
 * @file
 * VuoHidControl implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

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
	return (VuoHidControl){
		VuoJson_getObjectValue(VuoText, js, "name", NULL),
		VuoJson_getObjectValue(VuoInteger, js, "value", 0),
		VuoJson_getObjectValue(VuoInteger, js, "min", 0),
		VuoJson_getObjectValue(VuoInteger, js, "max", 0)
	};
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
	VuoType_returnInequality(VuoText,    a.name,  b.name);
	VuoType_returnInequality(VuoInteger, a.value, b.value);
	VuoType_returnInequality(VuoInteger, a.min,   b.min);
	VuoType_returnInequality(VuoInteger, a.max,   b.max);
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
		return strdup("No control");
}
