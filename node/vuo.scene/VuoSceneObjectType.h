/**
 * @file
 * VuoSceneObjectType C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoSceneObject.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoSceneObjectType VuoSceneObjectType
 * Type of 3d object.
 *
 * @{
 */

/**
 * The type of 3d object (Camera, Mesh, Group, etc).
 */
typedef enum
{
	VuoSceneObjectType_Any,
	VuoSceneObjectType_Group,
	VuoSceneObjectType_Mesh,
	VuoSceneObjectType_Camera,
	VuoSceneObjectType_Light
} VuoSceneObjectType;

#include "VuoList_VuoSceneObjectType.h"

/**
 * Create a VuoSceneObjectType from the internal VuoSceneObjectSubtype.
 */
VuoSceneObjectType VuoSceneObjectType_makeFromSubtype(VuoSceneObjectSubType subType);

VuoSceneObjectType VuoSceneObjectType_makeFromJson(struct json_object * js);
struct json_object * VuoSceneObjectType_getJson(const VuoSceneObjectType value);
VuoList_VuoSceneObjectType VuoSceneObjectType_getAllowedValues(void);
char * VuoSceneObjectType_getSummary(const VuoSceneObjectType value);

/**
 * Automatically generated function.
 */
///@{
char * VuoSceneObjectType_getString(const VuoSceneObjectType value);
void VuoSceneObjectType_retain(VuoSceneObjectType value);
void VuoSceneObjectType_release(VuoSceneObjectType value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
