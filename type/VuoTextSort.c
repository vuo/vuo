/**
 * @file
 * VuoTextSort implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoTextSort.h"
#include "VuoList_VuoTextSort.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Text Sort Type",
					 "description" : "Ways that text can be interpreted for sorting.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoList_VuoTextSort"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoTextSort
 * Decodes the JSON object to create a new value.
 */
VuoTextSort VuoTextSort_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	if (strcmp(valueAsString, "text case-sensitive") == 0)
		return VuoTextSort_TextCaseSensitive;
	else if (strcmp(valueAsString, "number") == 0)
		return VuoTextSort_Number;
	else if (strcmp(valueAsString, "date") == 0)
		return VuoTextSort_Date;

	return VuoTextSort_Text;
}

/**
 * @ingroup VuoTextSort
 * Encodes @a value as a JSON object.
 */
json_object * VuoTextSort_getJson(const VuoTextSort value)
{
	char *valueAsString = "text";

	if (value == VuoTextSort_TextCaseSensitive)
		valueAsString = "text case-sensitive";
	else if (value == VuoTextSort_Number)
		valueAsString = "number";
	else if (value == VuoTextSort_Date)
		valueAsString = "date";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoTextSort VuoTextSort_getAllowedValues(void)
{
	VuoList_VuoTextSort l = VuoListCreate_VuoTextSort();
	VuoListAppendValue_VuoTextSort(l, VuoTextSort_Text);
	VuoListAppendValue_VuoTextSort(l, VuoTextSort_TextCaseSensitive);
	VuoListAppendValue_VuoTextSort(l, VuoTextSort_Number);
	VuoListAppendValue_VuoTextSort(l, VuoTextSort_Date);
	return l;
}

/**
 * @ingroup VuoTextSort
 * Returns a string representation of @a value.
 */
char * VuoTextSort_getSummary(const VuoTextSort value)
{
	char *valueAsString = "Text";

	if (value == VuoTextSort_TextCaseSensitive)
		valueAsString = "Text (case-sensitive)";
	else if (value == VuoTextSort_Number)
		valueAsString = "Number";
	else if (value == VuoTextSort_Date)
		valueAsString = "Date";

	return strdup(valueAsString);
}
