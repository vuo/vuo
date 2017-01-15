/**
 * @file
 * VuoSyphonServerDescription C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOSYPHONSERVERDESCRIPTION_H
#define VUOSYPHONSERVERDESCRIPTION_H

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
 */
typedef struct
{
	VuoText serverUUID;
	VuoText serverName;
	VuoText applicationName;

} VuoSyphonServerDescription;

VuoSyphonServerDescription VuoSyphonServerDescription_makeFromJson(struct json_object * js);
struct json_object * VuoSyphonServerDescription_getJson(const VuoSyphonServerDescription value);
char * VuoSyphonServerDescription_getSummary(const VuoSyphonServerDescription value);
bool VuoSyphonServerDescription_areEqual(const VuoSyphonServerDescription value1, const VuoSyphonServerDescription value2);

VuoSyphonServerDescription VuoSyphonServerDescription_make(VuoText serverUUID, VuoText serverName, VuoText applicationName);

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

#endif
