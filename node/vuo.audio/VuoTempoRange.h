/**
 * @file
 * VuoTempoRange C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoTempoRange;
#define VuoList_VuoTempoRange_TYPE_DEFINED
/// @}

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
	VuoTempoRange_Allegro
} VuoTempoRange;

VuoTempoRange VuoTempoRange_makeFromJson(struct json_object *js);
struct json_object *VuoTempoRange_getJson(const VuoTempoRange value);
VuoList_VuoTempoRange VuoTempoRange_getAllowedValues(void);
char *VuoTempoRange_getSummary(const VuoTempoRange value);

int VuoTempoRange_getBaseBPM(const VuoTempoRange value);

#define VuoTempoRange_SUPPORTS_COMPARISON
bool VuoTempoRange_areEqual(const VuoTempoRange valueA, const VuoTempoRange valueB);
bool VuoTempoRange_isLessThan(const VuoTempoRange valueA, const VuoTempoRange valueB);

/**
 * Automatically generated function.
 */
///@{
VuoTempoRange VuoTempoRange_makeFromString(const char *str);
char *VuoTempoRange_getString(const VuoTempoRange value);
void VuoTempoRange_retain(VuoTempoRange value);
void VuoTempoRange_release(VuoTempoRange value);
///@}

/**
 * @}
 */
