/**
 * @file
 * VuoLoopType C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLOOPTYPE_H
#define VUOLOOPTYPE_H

/// @{
typedef const struct VuoList_VuoLoopType_struct { void *l; } * VuoList_VuoLoopType;
#define VuoList_VuoLoopType_TYPE_DEFINED
/// @}

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

VuoLoopType VuoLoopType_makeFromJson(struct json_object * js);
struct json_object * VuoLoopType_getJson(const VuoLoopType value);
VuoList_VuoLoopType VuoLoopType_getAllowedValues(void);
char * VuoLoopType_getSummary(const VuoLoopType value);

/// @{
/**
 * Automatically generated function.
 */
VuoLoopType VuoLoopType_makeFromString(const char *str);
char * VuoLoopType_getString(const VuoLoopType value);
void VuoLoopType_retain(VuoLoopType value);
void VuoLoopType_release(VuoLoopType value);
/// @}

/**
 * @}
*/

#endif
