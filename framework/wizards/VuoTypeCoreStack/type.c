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
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "%TypeDisplayName%",
					 "description" : "%TypeDescription%",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
					 ]
				 });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "replaceThis" : -1
 *   }
 * }
 */
%TypeName% %TypeName%_makeFromJson(json_object *js)
{
	%TypeName% value = {-1};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "replaceThis", &o))
		value.replaceThis = VuoInteger_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *%TypeName%_getJson(const %TypeName% value)
{
	json_object *js = json_object_new_object();

	json_object *replaceThisObject = VuoInteger_getJson(value.replaceThis);
	json_object_object_add(js, "replaceThis", replaceThisObject);

	return js;
}

/**
 * Returns true if the two values are equal.
 */
bool %TypeName%_areEqual(const %TypeName% valueA, const %TypeName% valueB)
{
	return valueA.replaceThis == valueB.replaceThis;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool %TypeName%_isLessThan(const %TypeName% valueA, const %TypeName% valueB)
{
	return valueA.replaceThis < valueB.replaceThis;
}

/**
 * Returns a compact string representation of @c value.
 */
char *%TypeName%_getSummary(const %TypeName% value)
{
	return VuoText_format("%d", value.replaceThis);
}

/**
 * Returns a %TypeDisplayName% with the specified values.
 */
%TypeName% %TypeName%_make(VuoInteger replaceThis)
{
	%TypeName% value;
	value.replaceThis = replaceThis;
	return value;
}
