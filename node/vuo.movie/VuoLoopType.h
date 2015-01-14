/**
 * @file
 * VuoLoopType C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLOOPTYPE_H
#define VUOLOOPTYPE_H

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

VuoLoopType VuoLoopType_valueFromJson(struct json_object * js);
struct json_object * VuoLoopType_jsonFromValue(const VuoLoopType value);
char * VuoLoopType_summaryFromValue(const VuoLoopType value);

/// @{
/**
 * Automatically generated function.
 */
VuoLoopType VuoLoopType_valueFromString(const char *str);
char * VuoLoopType_stringFromValue(const VuoLoopType value);
/// @}

/**
 * @}
*/

#endif
