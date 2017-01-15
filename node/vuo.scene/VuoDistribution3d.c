/**
 * @file
 * VuoDistribution3d implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoDistribution3d.h"
#include "VuoList_VuoDistribution3d.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "3D Distribution",
					  "description" : "A distribution of points in 3D space.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoDistribution3d"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "cube-surface"
 * }
 */
VuoDistribution3d VuoDistribution3d_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoDistribution3d value = VuoDistribution3d_CubeVolume;

	if (strcmp(valueAsString, "cube-surface") == 0)
		value = VuoDistribution3d_CubeSurface;
	else if (strcmp(valueAsString, "sphere-volume") == 0)
		value = VuoDistribution3d_SphereVolume;
	else if (strcmp(valueAsString, "sphere-surface") == 0)
		value = VuoDistribution3d_SphereSurface;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoDistribution3d_getJson(const VuoDistribution3d value)
{
	char *valueAsString = "cube-volume";

	if (value == VuoDistribution3d_CubeSurface)
		valueAsString = "cube-surface";
	else if (value == VuoDistribution3d_SphereVolume)
		valueAsString = "sphere-volume";
	else if (value == VuoDistribution3d_SphereSurface)
		valueAsString = "sphere-surface";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoDistribution3d VuoDistribution3d_getAllowedValues(void)
{
	VuoList_VuoDistribution3d l = VuoListCreate_VuoDistribution3d();
	VuoListAppendValue_VuoDistribution3d(l, VuoDistribution3d_CubeVolume);
	VuoListAppendValue_VuoDistribution3d(l, VuoDistribution3d_CubeSurface);
	VuoListAppendValue_VuoDistribution3d(l, VuoDistribution3d_SphereVolume);
	VuoListAppendValue_VuoDistribution3d(l, VuoDistribution3d_SphereSurface);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoDistribution3d_getSummary(const VuoDistribution3d value)
{
	char *valueAsString = "Cube Volume";

	if (value == VuoDistribution3d_CubeSurface)
		valueAsString = "Cube Surface";
	else if (value == VuoDistribution3d_SphereVolume)
		valueAsString = "Sphere Volume";
	else if (value == VuoDistribution3d_SphereSurface)
		valueAsString = "Sphere Surface";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoDistribution3d_areEqual(const VuoDistribution3d valueA, const VuoDistribution3d valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoDistribution3d_isLessThan(const VuoDistribution3d valueA, const VuoDistribution3d valueB)
{
	return valueA < valueB;
}

