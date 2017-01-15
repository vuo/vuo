/**
 * @file
 * VuoVertexAttribute C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOVERTEXATTRIBUTE_H
#define VUOVERTEXATTRIBUTE_H

/// @{
typedef void * VuoList_VuoVertexAttribute;
#define VuoList_VuoVertexAttribute_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoVertexAttribute VuoVertexAttribute
 * Data associated with a vertex of a 3D mesh.
 *
 * @{
 */

/**
 * Data associated with a vertex of a 3D mesh.
 */
typedef enum
{
	VuoVertexAttribute_Position,
	VuoVertexAttribute_Normal,
	VuoVertexAttribute_Tangent,
	VuoVertexAttribute_Bitangent,
	VuoVertexAttribute_TextureCoordinate,
} VuoVertexAttribute;

VuoVertexAttribute VuoVertexAttribute_makeFromJson(struct json_object *js);
struct json_object *VuoVertexAttribute_getJson(const VuoVertexAttribute value);
VuoList_VuoVertexAttribute VuoVertexAttribute_getAllowedValues(void);
char *VuoVertexAttribute_getSummary(const VuoVertexAttribute value);

#define VuoVertexAttribute_SUPPORTS_COMPARISON
bool VuoVertexAttribute_areEqual(const VuoVertexAttribute valueA, const VuoVertexAttribute valueB);
bool VuoVertexAttribute_isLessThan(const VuoVertexAttribute valueA, const VuoVertexAttribute valueB);

/**
 * Automatically generated function.
 */
///@{
VuoVertexAttribute VuoVertexAttribute_makeFromString(const char *str);
char *VuoVertexAttribute_getString(const VuoVertexAttribute value);
void VuoVertexAttribute_retain(VuoVertexAttribute value);
void VuoVertexAttribute_release(VuoVertexAttribute value);
///@}

/**
 * @}
 */

#endif // VUOVERTEXATTRIBUTE_H

