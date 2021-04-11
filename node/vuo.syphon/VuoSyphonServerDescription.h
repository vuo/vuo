/**
 * @file
 * VuoSyphonServerDescription C type definition.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoText.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoSyphonServerDescription VuoSyphonServerDescription
 *
 *
 * @{
 */

/**
 * A struct containing the dictionary values of a Syphon Server Description.
 *
 * @version200Changed{Added `useWildcard`.}
 */
typedef struct
{
	VuoText serverUUID;
	VuoText serverName;
	VuoText applicationName;
	bool useWildcard;
} VuoSyphonServerDescription;

VuoSyphonServerDescription VuoSyphonServerDescription_makeFromJson(struct json_object * js);
struct json_object * VuoSyphonServerDescription_getJson(const VuoSyphonServerDescription value);
char * VuoSyphonServerDescription_getSummary(const VuoSyphonServerDescription value);

#define VuoSyphonServerDescription_SUPPORTS_COMPARISON
bool VuoSyphonServerDescription_areEqual(const VuoSyphonServerDescription value1, const VuoSyphonServerDescription value2);
bool VuoSyphonServerDescription_isLessThan(const VuoSyphonServerDescription a, const VuoSyphonServerDescription b);

VuoSyphonServerDescription VuoSyphonServerDescription_make(VuoText serverUUID, VuoText serverName, VuoText applicationName, bool useWildcard);

/// @{
/**
 * Automatically generated function.
 */
VuoSyphonServerDescription VuoSyphonServerDescription_makeFromString(const char * initializer);
char * VuoSyphonServerDescription_getString(const VuoSyphonServerDescription value);
void VuoSyphonServerDescription_retain(VuoSyphonServerDescription value);
void VuoSyphonServerDescription_release(VuoSyphonServerDescription value);
/// @}

/**
 * @}
 */
