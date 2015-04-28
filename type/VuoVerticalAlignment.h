/**
 * @file
 * VuoVerticalAlignment C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOVERTICALALIGNMENT_H
#define VUOVERTICALALIGNMENT_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoVerticalAlignment VuoVerticalAlignment
 * Vertical alignment.
 *
 * @{
 */

#include "VuoInteger.h"

/**
 * Vertical alignment.
 */
typedef enum
{
	VuoVerticalAlignment_Top,
	VuoVerticalAlignment_Center,
	VuoVerticalAlignment_Bottom
} VuoVerticalAlignment;

VuoVerticalAlignment VuoVerticalAlignment_valueFromJson(struct json_object * js);
struct json_object * VuoVerticalAlignment_jsonFromValue(const VuoVerticalAlignment value);
char * VuoVerticalAlignment_summaryFromValue(const VuoVerticalAlignment value);

/**
 * Automatically generated function.
 */
///@{
VuoVerticalAlignment VuoVerticalAlignment_valueFromString(const char *str);
char * VuoVerticalAlignment_stringFromValue(const VuoVerticalAlignment value);
void VuoVerticalAlignment_retain(VuoVerticalAlignment value);
void VuoVerticalAlignment_release(VuoVerticalAlignment value);
///@}

/**
 * @}
 */

#endif // VUOVERTICALALIGNMENT_H

