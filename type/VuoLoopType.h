/**
 * @file
 * VuoLoopType C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoLoopType_h
#define VuoLoopType_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoLoopType VuoLoopType
 * An enum defining different types of movie playback loops.
 *
 * @{
 */

/**
 * An enum defining different types of movie playback loops.
 */
typedef enum {
	VuoLoopType_Loop,
	VuoLoopType_Mirror,
	VuoLoopType_None
} VuoLoopType;

#include "VuoList_VuoLoopType.h"

VuoLoopType VuoLoopType_makeFromJson(struct json_object * js);
struct json_object * VuoLoopType_getJson(const VuoLoopType value);
VuoList_VuoLoopType VuoLoopType_getAllowedValues(void);
char * VuoLoopType_getSummary(const VuoLoopType value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoLoopType_getString(const VuoLoopType value);
void VuoLoopType_retain(VuoLoopType value);
void VuoLoopType_release(VuoLoopType value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif

#endif
