/**
 * @file
 * VuoVertexAttribute implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoVertexAttribute.h"
#include "VuoList_VuoVertexAttribute.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Vertex Attribute",
					  "description" : "Data associated with a vertex of a 3D mesh.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoVertexAttribute"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "normal"
 * }
 */
VuoVertexAttribute VuoVertexAttribute_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoVertexAttribute value = VuoVertexAttribute_Position;

	if (strcmp(valueAsString, "normal") == 0)
		value = VuoVertexAttribute_Normal;
	else if (strcmp(valueAsString, "tangent") == 0)
		value = VuoVertexAttribute_Tangent;
	else if (strcmp(valueAsString, "bitangent") == 0)
		value = VuoVertexAttribute_Bitangent;
	else if (strcmp(valueAsString, "textureCoordinate") == 0)
		value = VuoVertexAttribute_TextureCoordinate;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoVertexAttribute_getJson(const VuoVertexAttribute value)
{
	char *valueAsString = "position";

	if (value == VuoVertexAttribute_Normal)
		valueAsString = "normal";
	else if (value == VuoVertexAttribute_Tangent)
		valueAsString = "tangent";
	else if (value == VuoVertexAttribute_Bitangent)
		valueAsString = "bitangent";
	else if (value == VuoVertexAttribute_TextureCoordinate)
		valueAsString = "textureCoordinate";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoVertexAttribute VuoVertexAttribute_getAllowedValues(void)
{
	VuoList_VuoVertexAttribute l = VuoListCreate_VuoVertexAttribute();
	VuoListAppendValue_VuoVertexAttribute(l, VuoVertexAttribute_Position);
	VuoListAppendValue_VuoVertexAttribute(l, VuoVertexAttribute_Normal);
	VuoListAppendValue_VuoVertexAttribute(l, VuoVertexAttribute_Tangent);
	VuoListAppendValue_VuoVertexAttribute(l, VuoVertexAttribute_Bitangent);
	VuoListAppendValue_VuoVertexAttribute(l, VuoVertexAttribute_TextureCoordinate);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoVertexAttribute_getSummary(const VuoVertexAttribute value)
{
	char *valueAsString = "Position";

	if (value == VuoVertexAttribute_Normal)
		valueAsString = "Normal";
	else if (value == VuoVertexAttribute_Tangent)
		valueAsString = "Tangent";
	else if (value == VuoVertexAttribute_Bitangent)
		valueAsString = "Bitangent";
	else if (value == VuoVertexAttribute_TextureCoordinate)
		valueAsString = "Texture Coordinate";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoVertexAttribute_areEqual(const VuoVertexAttribute valueA, const VuoVertexAttribute valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoVertexAttribute_isLessThan(const VuoVertexAttribute valueA, const VuoVertexAttribute valueB)
{
	return valueA < valueB;
}

