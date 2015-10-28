/**
 * @file
 * VuoHorizontalSide C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOHORIZONTALSIDE_H
#define VUOHORIZONTALSIDE_H

/// @{
typedef const struct VuoList_VuoHorizontalSide_struct { void *l; } * VuoList_VuoHorizontalSide;
#define VuoList_VuoHorizontalSide_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoHorizontalSide VuoHorizontalSide
 * An enum defining direction on the horizontal axis (right or left).
 *
 * @{
 */

/**
 * Direction on the horizontal axis.
 */
typedef enum {
	VuoHorizontalSide_Left,			//  8 bits per channel (32 bits per RGBA pixel)
	VuoHorizontalSide_Right			// 16 bits per channel (64 bits per RGBA pixel)
} VuoHorizontalSide;

VuoHorizontalSide VuoHorizontalSide_valueFromJson(struct json_object * js);
struct json_object * VuoHorizontalSide_jsonFromValue(const VuoHorizontalSide value);
VuoList_VuoHorizontalSide VuoHorizontalSide_allowedValues(void);
char * VuoHorizontalSide_summaryFromValue(const VuoHorizontalSide value);

/**
 * Automatically generated function.
 */
///@{
VuoHorizontalSide VuoHorizontalSide_valueFromString(const char *str);
char * VuoHorizontalSide_stringFromValue(const VuoHorizontalSide value);
void VuoHorizontalSide_retain(VuoHorizontalSide value);
void VuoHorizontalSide_release(VuoHorizontalSide value);
///@}

/**
 * @}
 */

#endif // VUOHORIZONTALSIDE_H

