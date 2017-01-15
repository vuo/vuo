/**
 * @file
 * %TypeName% implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "%TypeName%.h"
#include "VuoList_%TypeName%.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "%TypeDisplayName%",
					 "description" : "%TypeDescription%",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoList_%TypeName%"
					 ]
				 });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "%Type1Key%"
 * }
 */
%TypeName% %TypeName%_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	%TypeName% value = %TypeName%_%Type0KeyCamelCase%;

	if (strcmp(valueAsString, "%Type1Key%") == 0)
		value = %TypeName%_%Type1KeyCamelCase%;
	else if (strcmp(valueAsString, "%Type2Key%") == 0)
		value = %TypeName%_%Type2KeyCamelCase%;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *%TypeName%_getJson(const %TypeName% value)
{
	char *valueAsString = "%Type0Key%";

	if (value == %TypeName%_%Type1KeyCamelCase%)
		valueAsString = "%Type1Key%";
	else if (value == %TypeName%_%Type2KeyCamelCase%)
		valueAsString = "%Type2Key%";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_%TypeName% %TypeName%_getAllowedValues(void)
{
	VuoList_%TypeName% l = VuoListCreate_%TypeName%();
	VuoListAppendValue_%TypeName%(l, %TypeName%_%Type0KeyCamelCase%);
	VuoListAppendValue_%TypeName%(l, %TypeName%_%Type1KeyCamelCase%);
	VuoListAppendValue_%TypeName%(l, %TypeName%_%Type2KeyCamelCase%);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *%TypeName%_getSummary(const %TypeName% value)
{
	char *valueAsString = "%Type0Summary%";

	if (value == %TypeName%_%Type1KeyCamelCase%)
		valueAsString = "%Type1Summary%";
	else if (value == %TypeName%_%Type2KeyCamelCase%)
		valueAsString = "%Type2Summary%";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool %TypeName%_areEqual(const %TypeName% valueA, const %TypeName% valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool %TypeName%_isLessThan(const %TypeName% valueA, const %TypeName% valueB)
{
	return valueA < valueB;
}
