/**
 * @file
 * VuoNumberFormat C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoReal.h"
#include "VuoText.h"

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

#define VuoNumberFormat_SUPPORTS_COMPARISON
#include "VuoList_VuoNumberFormat.h"

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

#ifdef __cplusplus
}
#endif
