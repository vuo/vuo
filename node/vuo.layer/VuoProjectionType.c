/**
 * @file
 * VuoProjectionType implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoProjectionType.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Projection Type",
					 "description" : "Describes how an image is transformed when rendering with a perspective.",
					 "keywords" : [ "scale", "distance", "project", "map", "mapping" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoProjectionType"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoProjectionType
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoProjectionType.
 */
VuoProjectionType VuoProjectionType_makeFromJson(json_object *js)
{
	const char *valueAsString = "";

	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoProjectionType value = VuoProjectionType_Perspective;

	if (! strcmp(valueAsString, "perspective"))
		value = VuoProjectionType_Perspective;
	else if (! strcmp(valueAsString, "affine"))
		value = VuoProjectionType_Affine;

	return value;
}

/**
 * @ingroup VuoProjectionType
 * Encodes @c value as a JSON object.
 */
json_object * VuoProjectionType_getJson(const VuoProjectionType value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoProjectionType_Perspective:
			valueAsString = "perspective";
			break;
		case VuoProjectionType_Affine:
			valueAsString = "affine";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoProjectionType VuoProjectionType_getAllowedValues(void)
{
	VuoList_VuoProjectionType l = VuoListCreate_VuoProjectionType();
	VuoListAppendValue_VuoProjectionType(l, VuoProjectionType_Perspective);
	VuoListAppendValue_VuoProjectionType(l, VuoProjectionType_Affine);
	return l;
}

/**
 * @ingroup VuoProjectionType
 * Same as @c %VuoProjectionType_getString() but with some capitilization involved.
 */
char * VuoProjectionType_getSummary(const VuoProjectionType value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoProjectionType_Perspective:
			valueAsString = "Perspective";
			break;
		case VuoProjectionType_Affine:
			valueAsString = "Affine";
			break;
	}

	return strdup(valueAsString);
}
