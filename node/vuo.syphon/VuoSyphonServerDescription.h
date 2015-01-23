/**
 * @file
 * VuoSyphonServerDescription C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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

VuoSyphonServerDescription VuoSyphonServerDescription_valueFromJson(struct json_object * js);
struct json_object * VuoSyphonServerDescription_jsonFromValue(const VuoSyphonServerDescription value);
char * VuoSyphonServerDescription_summaryFromValue(const VuoSyphonServerDescription value);

VuoSyphonServerDescription VuoSyphonServerDescription_make(VuoText serverUUID, VuoText serverName, VuoText applicationName);

/// @{
/**
 * Automatically generated function.
 */
VuoSyphonServerDescription VuoSyphonServerDescription_valueFromString(const char * initializer);
char * VuoSyphonServerDescription_stringFromValue(const VuoSyphonServerDescription value);
void VuoSyphonServerDescription_retain(VuoSyphonServerDescription value);
void VuoSyphonServerDescription_release(VuoSyphonServerDescription value);
/// @}

/**
 * @}
 */

#endif
