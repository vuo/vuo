/**
 * @file
 * VuoTempoRange C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoTempoRange VuoTempoRange
 * A range of BPM values.
 *
 * @{
 */

/**
 * A range of BPM values.
 */
typedef enum
{
	VuoTempoRange_Andante,
	VuoTempoRange_Moderato,
	VuoTempoRange_Allegro,
	VuoTempoRange_Presto,
	VuoTempoRange_Prestissimo
} VuoTempoRange;

#define VuoTempoRange_SUPPORTS_COMPARISON
#include "VuoList_VuoTempoRange.h"

VuoTempoRange VuoTempoRange_makeFromJson(struct json_object *js);
struct json_object *VuoTempoRange_getJson(const VuoTempoRange value);
VuoList_VuoTempoRange VuoTempoRange_getAllowedValues(void);
char *VuoTempoRange_getSummary(const VuoTempoRange value);

int VuoTempoRange_getBaseBPM(const VuoTempoRange value);

bool VuoTempoRange_areEqual(const VuoTempoRange valueA, const VuoTempoRange valueB);
bool VuoTempoRange_isLessThan(const VuoTempoRange valueA, const VuoTempoRange valueB);

/**
 * Automatically generated function.
 */
///@{
char *VuoTempoRange_getString(const VuoTempoRange value);
void VuoTempoRange_retain(VuoTempoRange value);
void VuoTempoRange_release(VuoTempoRange value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
