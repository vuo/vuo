/**
 * @file
 * VuoTextCase implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>
#include "type.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Text Case",
					 "description" : "Describes text casing styles.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoTextCase"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoTextCase
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoTextCase.
 */
VuoTextCase VuoTextCase_makeFromJson(json_object *js)
{
	const char *valueAsString = "";

	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	if( strcmp(valueAsString, "lower") == 0 )
		return VuoTextCase_LowercaseAll;
	else if( strcmp(valueAsString, "upper") == 0 )
		return VuoTextCase_UppercaseAll;
	else if( strcmp(valueAsString, "word") == 0 )
		return VuoTextCase_UppercaseFirstLetterWord;

	return VuoTextCase_UppercaseFirstLetterSentence;
}

/**
 * @ingroup VuoTextCase
 * Encodes @c value as a JSON object.
 */
json_object * VuoTextCase_getJson(const VuoTextCase value)
{
	char *valueAsString = "sentence";

	if( value == VuoTextCase_LowercaseAll )
		valueAsString = "lower";
	else if( value == VuoTextCase_UppercaseAll )
		valueAsString = "upper";
	else if( value == VuoTextCase_UppercaseFirstLetterWord )
		valueAsString = "word";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoTextCase VuoTextCase_getAllowedValues(void)
{
	VuoList_VuoTextCase l = VuoListCreate_VuoTextCase();
	VuoListAppendValue_VuoTextCase(l, VuoTextCase_LowercaseAll);
	VuoListAppendValue_VuoTextCase(l, VuoTextCase_UppercaseAll);
	VuoListAppendValue_VuoTextCase(l, VuoTextCase_UppercaseFirstLetterWord);
	VuoListAppendValue_VuoTextCase(l, VuoTextCase_UppercaseFirstLetterSentence);
	return l;
}

/**
 * @ingroup VuoTextCase
 * Same as @c %VuoTextCase_getString()
 */
char * VuoTextCase_getSummary(const VuoTextCase value)
{
	char *valueAsString = "Uppercase first letter of each sentence";

	if( value == VuoTextCase_LowercaseAll )
		valueAsString = "Lowercase all";
	else if( value == VuoTextCase_UppercaseAll )
		valueAsString = "Uppercase all";
	else if( value == VuoTextCase_UppercaseFirstLetterWord )
		valueAsString = "Uppercase first letter of each word";

	return strdup(valueAsString);
}
