/**
 * @file
 * VuoDiode C type definition.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoDiode;
#define VuoList_VuoDiode_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoDiode VuoDiode
 * A method for handling signal polarity.
 *
 * @{
 */

/**
 * A method for handling signal polarity.
 */
typedef enum
{
	VuoDiode_Unipolar, // min  0.0, center 0.5, max 1.0
	VuoDiode_Bipolar,  // min -1.0, center 0.0, max 1.0
	VuoDiode_Absolute,
} VuoDiode;

VuoDiode VuoDiode_makeFromJson(struct json_object *js);
struct json_object *VuoDiode_getJson(const VuoDiode value);
VuoList_VuoDiode VuoDiode_getAllowedValues(void);
char *VuoDiode_getSummary(const VuoDiode value);

#define VuoDiode_SUPPORTS_COMPARISON
bool VuoDiode_areEqual(const VuoDiode valueA, const VuoDiode valueB);
bool VuoDiode_isLessThan(const VuoDiode valueA, const VuoDiode valueB);

/**
 * Automatically generated function.
 */
///@{
VuoDiode VuoDiode_makeFromString(const char *str);
char *VuoDiode_getString(const VuoDiode value);
void VuoDiode_retain(VuoDiode value);
void VuoDiode_release(VuoDiode value);
///@}

/**
 * @}
 */
