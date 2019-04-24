/**
 * @file
 * VuoBlackmagicConnection C type definition.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_VuoBlackmagicConnection;
#define VuoList_VuoBlackmagicConnection_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoBlackmagicConnection VuoBlackmagicConnection
 * Blackmagic video input port
 *
 * @{
 */

/**
 * Blackmagic video input port
 */
typedef enum
{
	VuoBlackmagicConnection_Composite,
	VuoBlackmagicConnection_SVideo,
	VuoBlackmagicConnection_Component,
	VuoBlackmagicConnection_HDMI,
	VuoBlackmagicConnection_SDI,
	VuoBlackmagicConnection_SDIOptical,
} VuoBlackmagicConnection;

VuoBlackmagicConnection VuoBlackmagicConnection_makeFromJson(struct json_object *js);
struct json_object *VuoBlackmagicConnection_getJson(const VuoBlackmagicConnection value);
VuoList_VuoBlackmagicConnection VuoBlackmagicConnection_getAllowedValues(void);
char *VuoBlackmagicConnection_getSummary(const VuoBlackmagicConnection value);

uint32_t VuoBlackmagicConnection_getBMDVideoConnection(const VuoBlackmagicConnection value);

#define VuoBlackmagicConnection_SUPPORTS_COMPARISON
bool VuoBlackmagicConnection_areEqual(const VuoBlackmagicConnection valueA, const VuoBlackmagicConnection valueB);
bool VuoBlackmagicConnection_isLessThan(const VuoBlackmagicConnection valueA, const VuoBlackmagicConnection valueB);

/**
 * Automatically generated function.
 */
///@{
VuoBlackmagicConnection VuoBlackmagicConnection_makeFromString(const char *str);
char *VuoBlackmagicConnection_getString(const VuoBlackmagicConnection value);
void VuoBlackmagicConnection_retain(VuoBlackmagicConnection value);
void VuoBlackmagicConnection_release(VuoBlackmagicConnection value);
///@}

/**
 * @}
 */
