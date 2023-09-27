/**
 * @file
 * VuoOscType C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoOscType VuoOscType
 * An OSC data type.
 *
 * @{
 */

/**
 * An OSC data type.
 */
typedef enum
{
	VuoOscType_Auto,
	VuoOscType_Int32,
	VuoOscType_Float32
} VuoOscType;

#define VuoOscType_SUPPORTS_COMPARISON
#include "VuoList_VuoOscType.h"

VuoOscType VuoOscType_makeFromJson(struct json_object *js);
struct json_object *VuoOscType_getJson(const VuoOscType value);
VuoList_VuoOscType VuoOscType_getAllowedValues(void);
char *VuoOscType_getSummary(const VuoOscType value);

bool VuoOscType_areEqual(const VuoOscType valueA, const VuoOscType valueB);
bool VuoOscType_isLessThan(const VuoOscType valueA, const VuoOscType valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoOscType_getString(const VuoOscType value);
void VuoOscType_retain(VuoOscType value);
void VuoOscType_release(VuoOscType value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
