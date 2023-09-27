/**
 * @file
 * VuoWrapMode C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoWrapMode_h
#define VuoWrapMode_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoWrapMode VuoWrapMode
 * An enum defining different types of wrapping.
 *
 * @{
 */

/**
 * An enum defining different types of wrapping.
 */
typedef enum {
	VuoWrapMode_Wrap,
	VuoWrapMode_Saturate
} VuoWrapMode;

#include "VuoList_VuoWrapMode.h"

VuoWrapMode VuoWrapMode_makeFromJson(struct json_object * js);
struct json_object * VuoWrapMode_getJson(const VuoWrapMode value);
VuoList_VuoWrapMode VuoWrapMode_getAllowedValues(void);
char * VuoWrapMode_getSummary(const VuoWrapMode value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoWrapMode_getString(const VuoWrapMode value);
void VuoWrapMode_retain(VuoWrapMode value);
void VuoWrapMode_release(VuoWrapMode value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif

#endif
