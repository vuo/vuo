/**
 * @file
 * VuoVerticalAlignment C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOVERTICALALIGNMENT_H
#define VUOVERTICALALIGNMENT_H

/// @{
typedef const struct VuoList_VuoVerticalAlignment_struct { void *l; } * VuoList_VuoVerticalAlignment;
#define VuoList_VuoVerticalAlignment_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoVerticalAlignment VuoVerticalAlignment
 * Vertical alignment.
 *
 * @{
 */

/**
 * Vertical alignment.
 */
typedef enum
{
	VuoVerticalAlignment_Top,
	VuoVerticalAlignment_Center,
	VuoVerticalAlignment_Bottom
} VuoVerticalAlignment;

VuoVerticalAlignment VuoVerticalAlignment_makeFromJson(struct json_object * js);
struct json_object * VuoVerticalAlignment_getJson(const VuoVerticalAlignment value);
VuoList_VuoVerticalAlignment VuoVerticalAlignment_getAllowedValues(void);
char * VuoVerticalAlignment_getSummary(const VuoVerticalAlignment value);

/**
 * Automatically generated function.
 */
///@{
VuoVerticalAlignment VuoVerticalAlignment_makeFromString(const char *str);
char * VuoVerticalAlignment_getString(const VuoVerticalAlignment value);
void VuoVerticalAlignment_retain(VuoVerticalAlignment value);
void VuoVerticalAlignment_release(VuoVerticalAlignment value);
///@}

/**
 * @}
 */

#endif // VUOVERTICALALIGNMENT_H

