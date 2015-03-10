/**
 * @file
 * VuoCurveDomain implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoCurveDomain.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Curve Domain",
					 "description" : "Domain that can be applied to a curve.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoCurveDomain
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoCurveDomain.
 */
VuoCurveDomain VuoCurveDomain_valueFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoCurveDomain domain = VuoCurveDomain_Clamp;

	if (strcmp(valueAsString, "clamp") == 0) {
		domain = VuoCurveDomain_Clamp;
	} else if (strcmp(valueAsString, "infinite") == 0) {
		domain = VuoCurveDomain_Infinite;
	} else if (strcmp(valueAsString, "wrap") == 0) {
		domain = VuoCurveDomain_Wrap;
	} else if (strcmp(valueAsString, "mirror") == 0) {
		domain = VuoCurveDomain_Mirror;
	}

	return domain;
}

/**
 * @ingroup VuoCurveDomain
 * Encodes @c value as a JSON object.
 */
json_object * VuoCurveDomain_jsonFromValue(const VuoCurveDomain value)
{
	char * valueAsString = "";

	switch (value) {
		case VuoCurveDomain_Clamp:
			valueAsString = "clamp";
			break;
		case VuoCurveDomain_Infinite:
			valueAsString = "infinite";
			break;
		case VuoCurveDomain_Wrap:
			valueAsString = "wrap";
			break;
		case VuoCurveDomain_Mirror:
			valueAsString = "mirror";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * @ingroup VuoCurveDomain
 * Same as @c %VuoCurveDomain_stringFromValue()
 */
char * VuoCurveDomain_summaryFromValue(const VuoCurveDomain value)
{
	char * valueAsString = "";

	switch (value) {
		case VuoCurveDomain_Clamp:
			valueAsString = "Clamp";
			break;
		case VuoCurveDomain_Infinite:
			valueAsString = "Infinite";
			break;
		case VuoCurveDomain_Wrap:
			valueAsString = "Wrap";
			break;
		case VuoCurveDomain_Mirror:
			valueAsString = "Mirror";
			break;
	}

	return valueAsString;
}
