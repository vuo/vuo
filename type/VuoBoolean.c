/**
 * @file
 * VuoBoolean implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoBoolean.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Boolean",
					 "description" : "A boolean (true/false) value.",
					 "keywords" : [ "true", "false" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoBoolean
 * Decodes the JSON object @c js, expected to contain a boolean, to create a new @c VuoBoolean.
 */
VuoBoolean VuoBoolean_valueFromJson(json_object * js)
{
	return json_object_get_boolean(js);
}

/**
 * @ingroup VuoBoolean
 * Encodes @c value as a JSON object.
 */
json_object * VuoBoolean_jsonFromValue(const VuoBoolean value)
{
	return json_object_new_boolean(value);
}

/**
 * @ingroup VuoBoolean
 * Same as @c %VuoBoolean_stringFromValue() — always shows the full value, since it's guaranteed to be pretty short.
 */
char * VuoBoolean_summaryFromValue(const VuoBoolean value)
{
	// Always show the full value, since it's guaranteed to be pretty short.
	return VuoBoolean_stringFromValue(value);
}
