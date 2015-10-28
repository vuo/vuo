/**
 * @file
 * VuoVerticalReflection C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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

VuoVerticalReflection VuoVerticalReflection_valueFromJson(struct json_object * js);
struct json_object * VuoVerticalReflection_jsonFromValue(const VuoVerticalReflection value);
VuoList_VuoVerticalReflection VuoVerticalReflection_allowedValues(void);
char * VuoVerticalReflection_summaryFromValue(const VuoVerticalReflection value);

/// @{
/**
 * Automatically generated function.
 */
VuoVerticalReflection VuoVerticalReflection_valueFromString(const char *str);
char * VuoVerticalReflection_stringFromValue(const VuoVerticalReflection value);
/// @}

/**
 * @}
*/

#endif
