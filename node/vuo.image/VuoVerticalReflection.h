/**
 * @file
 * VuoVerticalReflection C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOVERTICALREFLECTION_H
#define VUOVERTICALREFLECTION_H

/// @{
typedef const struct VuoList_VuoVerticalReflection_struct { void *l; } * VuoList_VuoVerticalReflection;
#define VuoList_VuoVerticalReflection_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoVerticalReflection VuoVerticalReflection
 * Options for mirroring an image along the y-axis.
 *
 * @{
 */

/**
 * An enum defining different types of color masks.
 */
typedef enum {
	VuoVerticalReflection_None,
	VuoVerticalReflection_Top,
	VuoVerticalReflection_Bottom
} VuoVerticalReflection;

VuoVerticalReflection VuoVerticalReflection_makeFromJson(struct json_object * js);
struct json_object * VuoVerticalReflection_getJson(const VuoVerticalReflection value);
VuoList_VuoVerticalReflection VuoVerticalReflection_getAllowedValues(void);
char * VuoVerticalReflection_getSummary(const VuoVerticalReflection value);

/// @{
/**
 * Automatically generated function.
 */
VuoVerticalReflection VuoVerticalReflection_makeFromString(const char *str);
char * VuoVerticalReflection_getString(const VuoVerticalReflection value);
/// @}

/**
 * @}
*/

#endif
