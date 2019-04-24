/**
 * @file
 * VuoTextComparison implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoTextComparison.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Text Comparison",
					 "description" : "Parameters for comparing two texts",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoText"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoTextComparison
 * Decodes the JSON object to create a new value.
 */
VuoTextComparison VuoTextComparison_makeFromJson(json_object *js)
{
	VuoTextComparison value = { VuoTextComparison_Equals, true };

	json_object *o;
	if (json_object_object_get_ex(js, "type", &o))
	{
		const char *typeAsString = json_object_get_string(o);
		if (! strcmp(typeAsString, "equals"))
			value.type = VuoTextComparison_Equals;
		else if (! strcmp(typeAsString, "contains"))
			value.type = VuoTextComparison_Contains;
		else if (! strcmp(typeAsString, "beginsWith"))
			value.type = VuoTextComparison_BeginsWith;
		else if (! strcmp(typeAsString, "endsWith"))
			value.type = VuoTextComparison_EndsWith;
	}

	if (json_object_object_get_ex(js, "isCaseSensitive", &o))
		value.isCaseSensitive = json_object_get_boolean(o);

	return value;
}

/**
 * @ingroup VuoTextComparison
 * Encodes @a value as a JSON object.
 */
json_object * VuoTextComparison_getJson(const VuoTextComparison value)
{
	json_object *js = json_object_new_object();

	const char *typeAsString;
	switch (value.type)
	{
		case VuoTextComparison_Equals:
			typeAsString = "equals";
			break;
		case VuoTextComparison_Contains:
			typeAsString = "contains";
			break;
		case VuoTextComparison_BeginsWith:
			typeAsString = "beginsWith";
			break;
		case VuoTextComparison_EndsWith:
			typeAsString = "endsWith";
			break;
	}
	json_object_object_add(js, "type", json_object_new_string(typeAsString));

	json_object_object_add(js, "isCaseSensitive", json_object_new_boolean(value.isCaseSensitive));

	return js;
}

/**
 * @ingroup VuoTextComparison
 * Returns a string representation of @a value.
 */
char * VuoTextComparison_getSummary(const VuoTextComparison value)
{
	const char *typeAsString;
	switch (value.type)
	{
		case VuoTextComparison_Equals:
			typeAsString = "Equals";
			break;
		case VuoTextComparison_Contains:
			typeAsString = "Contains";
			break;
		case VuoTextComparison_BeginsWith:
			typeAsString = "Begins with";
			break;
		case VuoTextComparison_EndsWith:
			typeAsString = "Ends with";
			break;
	}

	const char *caseAsString = (value.isCaseSensitive ? "case-sensitive" : "not case-sensitive");

	return VuoText_format("%s (%s)", typeAsString, caseAsString);
}
