/**
 * @file
 * VuoSceneObject C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOSCENEOBJECT_H
#define VUOSCENEOBJECT_H

#include "VuoVertices.h"
#include "VuoShader.h"
#include "VuoTransform.h"
#include "VuoList_VuoVertices.h"

/// @{
typedef void * VuoList_VuoSceneObject;
#define VuoList_VuoSceneObject_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoSceneObject VuoSceneObject
 * A renderable 3D Object.
 *
 * @{
 */

/**
 * A renderable 3D Object.
 */
typedef struct VuoSceneObject
{
	VuoList_VuoVertices verticesList;
	VuoShader shader;
	VuoTransform transform;

	VuoList_VuoSceneObject childObjects;
} VuoSceneObject;

VuoSceneObject VuoSceneObject_makeEmpty(void);
VuoSceneObject VuoSceneObject_make(VuoList_VuoVertices verticesList, VuoShader shader, VuoTransform transform, VuoList_VuoSceneObject childObjects);
VuoSceneObject VuoSceneObject_valueFromJson(struct json_object * js);
struct json_object * VuoSceneObject_jsonFromValue(const VuoSceneObject value);
char * VuoSceneObject_summaryFromValue(const VuoSceneObject value);

///@{
/**
 * Automatically generated function.
 */
VuoSceneObject VuoSceneObject_valueFromString(const char *str);
char * VuoSceneObject_stringFromValue(const VuoSceneObject value);
void VuoSceneObject_retain(VuoSceneObject value);
void VuoSceneObject_release(VuoSceneObject value);
///@}

/**
 * @}
 */

#endif
