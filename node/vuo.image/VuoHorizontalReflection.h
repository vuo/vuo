/**
 * @file
 * VuoHorizontalReflection C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOHORIZONTALREFLECTION_H
#define VUOHORIZONTALREFLECTION_H

/// @{
typedef const struct VuoList_VuoHorizontalReflection_struct { void *l; } * VuoList_VuoHorizontalReflection;
#define VuoList_VuoHorizontalReflection_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoHorizontalReflection VuoHorizontalReflection
 * Options for mirroring an image along the y-axis.
 *
 * @{
 */

/**
 * An enum defining different types of color masks.
 */
typedef enum {
	VuoHorizontalReflection_None,
	VuoHorizontalReflection_Left,
	VuoHorizontalReflection_Right
} VuoHorizontalReflection;

VuoHorizontalReflection VuoHorizontalReflection_valueFromJson(struct json_object * js);
struct json_object * VuoHorizontalReflection_jsonFromValue(const VuoHorizontalReflection value);
VuoList_VuoHorizontalReflection VuoHorizontalReflection_allowedValues(void);
char * VuoHorizontalReflection_summaryFromValue(const VuoHorizontalReflection value);

/// @{
/**
 * Automatically generated function.
 */
VuoHorizontalReflection VuoHorizontalReflection_valueFromString(const char *str);
char * VuoHorizontalReflection_stringFromValue(const VuoHorizontalReflection value);
/// @}

/**
 * @}
*/

#endif
