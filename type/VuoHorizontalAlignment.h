/**
 * @file
 * VuoHorizontalAlignment C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOHORIZONTALALIGNMENT_H
#define VUOHORIZONTALALIGNMENT_H

/// @{
typedef void * VuoList_VuoHorizontalAlignment;
#define VuoList_VuoHorizontalAlignment_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoHorizontalAlignment VuoHorizontalAlignment
 * Horizontal alignment.
 *
 * @{
 */

/**
 * Horizontal alignment.
 */
typedef enum
{
	VuoHorizontalAlignment_Left,
	VuoHorizontalAlignment_Center,
	VuoHorizontalAlignment_Right
} VuoHorizontalAlignment;

VuoHorizontalAlignment VuoHorizontalAlignment_valueFromJson(struct json_object * js);
struct json_object * VuoHorizontalAlignment_jsonFromValue(const VuoHorizontalAlignment value);
VuoList_VuoHorizontalAlignment VuoHorizontalAlignment_allowedValues(void);
char * VuoHorizontalAlignment_summaryFromValue(const VuoHorizontalAlignment value);

/**
 * Automatically generated function.
 */
///@{
VuoHorizontalAlignment VuoHorizontalAlignment_valueFromString(const char *str);
char * VuoHorizontalAlignment_stringFromValue(const VuoHorizontalAlignment value);
void VuoHorizontalAlignment_retain(VuoHorizontalAlignment value);
void VuoHorizontalAlignment_release(VuoHorizontalAlignment value);
///@}

/**
 * @}
 */

#endif // VUOHORIZONTALALIGNMENT_H

