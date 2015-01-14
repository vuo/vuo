/**
 * @file
 * VuoInteger implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoInteger.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Integer",
					 "description" : "A signed 64-bit integer.",
					 "keywords" : [ "number", "signed" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoInteger
 * Decodes the JSON object @c js, expected to contain a 64-bit integer, to create a new value.
 */
VuoInteger VuoInteger_valueFromJson(json_object * js)
{
	return json_object_get_int64(js);
}

/**
 * @ingroup VuoInteger
 * Encodes @c value as a JSON object.
 */
json_object * VuoInteger_jsonFromValue(const VuoInteger value)
{
	return json_object_new_int64(value);
}

/**
 * @ingroup VuoInteger
 * Same as @c %VuoInteger_stringFromValue() — always shows the full value, since it's guaranteed to be pretty short.
 */
char * VuoInteger_summaryFromValue(const VuoInteger value)
{
	return VuoInteger_stringFromValue(value);
}
