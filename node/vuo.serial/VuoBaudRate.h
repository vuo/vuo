/**
 * @file
 * VuoBaudRate C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoBaudRate;
#define VuoList_VuoBaudRate_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoBaudRate VuoBaudRate
 * The speed of a serial connection.
 *
 * @{
 */

/**
 * The speed of a serial connection.
 */
typedef enum
{
	VuoBaudRate_200 = 200,
	VuoBaudRate_300 = 300,
	VuoBaudRate_600 = 600,
	VuoBaudRate_1200 = 1200,
	VuoBaudRate_1800 = 1800,
	VuoBaudRate_2400 = 2400,
	VuoBaudRate_4800 = 4800,
	VuoBaudRate_9600 = 9600,
	VuoBaudRate_14400 = 14400,
	VuoBaudRate_19200 = 19200,
	VuoBaudRate_28800 = 28800,
	VuoBaudRate_38400 = 38400,
	VuoBaudRate_57600 = 57600,
	VuoBaudRate_115200 = 115200,
	VuoBaudRate_230400 = 230400,
	VuoBaudRate_460800 = 460800,
	VuoBaudRate_921600 = 921600,
} VuoBaudRate;

VuoBaudRate VuoBaudRate_makeFromJson(struct json_object *js);
struct json_object *VuoBaudRate_getJson(const VuoBaudRate value);
VuoList_VuoBaudRate VuoBaudRate_getAllowedValues(void);
char *VuoBaudRate_getSummary(const VuoBaudRate value);

#define VuoBaudRate_SUPPORTS_COMPARISON
bool VuoBaudRate_areEqual(const VuoBaudRate valueA, const VuoBaudRate valueB);
bool VuoBaudRate_isLessThan(const VuoBaudRate valueA, const VuoBaudRate valueB);

/**
 * Automatically generated function.
 */
///@{
VuoBaudRate VuoBaudRate_makeFromString(const char *str);
char *VuoBaudRate_getString(const VuoBaudRate value);
void VuoBaudRate_retain(VuoBaudRate value);
void VuoBaudRate_release(VuoBaudRate value);
///@}

/**
 * @}
 */

