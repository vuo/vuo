/**
 * @file
 * VuoDisplacement C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUODISPLACEMENT_H
#define VUODISPLACEMENT_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoDisplacement VuoDisplacement
 * The direction in which to move vertices.
 *
 * @{
 */

#include "VuoInteger.h"

/**
 * The direction in which to move vertices.
 */
typedef enum {
	VuoDisplacement_Transverse,
	VuoDisplacement_Longitudinal
} VuoDisplacement;

VuoDisplacement VuoDisplacement_valueFromJson(struct json_object * js);
struct json_object * VuoDisplacement_jsonFromValue(const VuoDisplacement value);
char * VuoDisplacement_summaryFromValue(const VuoDisplacement value);

/**
 * Automatically generated function.
 */
///@{
VuoDisplacement VuoDisplacement_valueFromString(const char *str);
char * VuoDisplacement_stringFromValue(const VuoDisplacement value);
void VuoDisplacement_retain(VuoDisplacement value);
void VuoDisplacement_release(VuoDisplacement value);
///@}

/**
 * @}
 */

#endif // VUODISPLACEMENT_H
