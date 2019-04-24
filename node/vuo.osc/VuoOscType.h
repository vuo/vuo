/**
 * @file
 * VuoOscType C type definition.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoOscType;
#define VuoList_VuoOscType_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoOscType VuoOscType
 * An OSC data type.
 *
 * @{
 */

/**
 * An OSC data type.
 */
typedef enum
{
	VuoOscType_Auto,
	VuoOscType_Int32,
	VuoOscType_Float32
} VuoOscType;

VuoOscType VuoOscType_makeFromJson(struct json_object *js);
struct json_object *VuoOscType_getJson(const VuoOscType value);
VuoList_VuoOscType VuoOscType_getAllowedValues(void);
char *VuoOscType_getSummary(const VuoOscType value);

#define VuoOscType_SUPPORTS_COMPARISON
bool VuoOscType_areEqual(const VuoOscType valueA, const VuoOscType valueB);
bool VuoOscType_isLessThan(const VuoOscType valueA, const VuoOscType valueB);

/**
 * Automatically generated function.
 */
///@{
VuoOscType VuoOscType_makeFromString(const char *str);
char *VuoOscType_getString(const VuoOscType value);
void VuoOscType_retain(VuoOscType value);
void VuoOscType_release(VuoOscType value);
///@}

/**
 * @}
 */
