/**
 * @file
 * VuoProjectionType C type definition.
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
 * @defgroup VuoProjectionType VuoProjectionType
 * An enum defining different projection mapping techniques.
 *
 * @{
 */

/**
 * An enum defining different projection mapping techniques.
 */
typedef enum {
	VuoProjectionType_Perspective,
	VuoProjectionType_Affine
} VuoProjectionType;

#include "VuoList_VuoProjectionType.h"

VuoProjectionType VuoProjectionType_makeFromJson(struct json_object * js);
struct json_object * VuoProjectionType_getJson(const VuoProjectionType value);
VuoList_VuoProjectionType VuoProjectionType_getAllowedValues(void);
char * VuoProjectionType_getSummary(const VuoProjectionType value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoProjectionType_getString(const VuoProjectionType value);
void VuoProjectionType_retain(VuoProjectionType value);
void VuoProjectionType_release(VuoProjectionType value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif
