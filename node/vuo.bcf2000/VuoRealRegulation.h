/**
 * @file
 * VuoRealRegulation C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoRealRegulation VuoRealRegulation
 * Parameters describing how to regulate a real number.
 *
 * @{
 */

#include "VuoReal.h"
#include "VuoText.h"

/**
 * Parameters describing how to regulate a real number.
 */
typedef struct
{
	VuoText name;
	VuoReal minimumValue;
	VuoReal maximumValue;
	VuoReal defaultValue;
	VuoReal smoothDuration;
} VuoRealRegulation;

VuoRealRegulation VuoRealRegulation_makeFromJson(struct json_object * js);
struct json_object * VuoRealRegulation_getJson(const VuoRealRegulation value);
char * VuoRealRegulation_getSummary(const VuoRealRegulation value);

VuoRealRegulation VuoRealRegulation_make(VuoText name, VuoReal minimumValue, VuoReal maximumValue, VuoReal defaultValue, VuoReal smoothDuration);

/**
 * Automatically generated function.
 */
///@{
VuoRealRegulation VuoRealRegulation_makeFromString(const char *str);
char * VuoRealRegulation_getString(const VuoRealRegulation value);
void VuoRealRegulation_retain(VuoRealRegulation value);
void VuoRealRegulation_release(VuoRealRegulation value);
///@}

/**
 * @}
 */


