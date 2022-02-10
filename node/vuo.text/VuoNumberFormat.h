/**
 * @file
 * VuoNumberFormat C type definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoReal.h"

/// @{ List type.
typedef void * VuoList_VuoNumberFormat;
#define VuoList_VuoNumberFormat_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoNumberFormat VuoNumberFormat
 * How to convert a number into text.
 *
 * @{
 */

/**
 * How to convert a number into text.
 */
typedef enum
{
	VuoNumberFormat_Decimal,
	VuoNumberFormat_Percentage,
	VuoNumberFormat_Currency
} VuoNumberFormat;

VuoNumberFormat VuoNumberFormat_makeFromJson(struct json_object *js);
struct json_object *VuoNumberFormat_getJson(const VuoNumberFormat value);
VuoList_VuoNumberFormat VuoNumberFormat_getAllowedValues(void);
char *VuoNumberFormat_getSummary(const VuoNumberFormat value);

VuoText VuoNumberFormat_format(VuoReal value,
	VuoNumberFormat format,
	VuoInteger minimumIntegerDigits,
	VuoInteger minimumDecimalPlaces,
	VuoInteger decimalPlaces,
	bool showThousandSeparator);

#define VuoNumberFormat_SUPPORTS_COMPARISON
bool VuoNumberFormat_areEqual(const VuoNumberFormat valueA, const VuoNumberFormat valueB);
bool VuoNumberFormat_isLessThan(const VuoNumberFormat valueA, const VuoNumberFormat valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoNumberFormat_getString(const VuoNumberFormat value);
void VuoNumberFormat_retain(VuoNumberFormat value);
void VuoNumberFormat_release(VuoNumberFormat value);
///@}

/**
 * @}
 */


