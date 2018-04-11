/**
 * @file
 * VuoOscMessage C type definition.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoText.h"
#include "VuoOscType.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoOscMessage VuoOscMessage
 * An OSC message.
 *
 * @{
 */

/// Maximum supported number of OSC message arguments.
#define VUOOSC_MAX_MESSAGE_ARGUMENTS 256

/**
 * An OSC message.
 */
typedef struct _VuoOscMessage
{
	VuoText address;

	unsigned int dataCount;
	struct json_object *data[VUOOSC_MAX_MESSAGE_ARGUMENTS];
	VuoOscType dataTypes[VUOOSC_MAX_MESSAGE_ARGUMENTS];
} *VuoOscMessage;

VuoOscMessage VuoOscMessage_make(VuoText address, unsigned int dataCount, struct json_object **data, VuoOscType *dataTypes);

VuoOscMessage VuoOscMessage_makeFromJson(struct json_object * js);
struct json_object * VuoOscMessage_getJson(const VuoOscMessage value);
char * VuoOscMessage_getSummary(const VuoOscMessage value);

///@{
/**
 * Automatically generated function.
 */
VuoOscMessage VuoOscMessage_makeFromString(const char *str);
char * VuoOscMessage_getString(const VuoOscMessage value);
void VuoOscMessage_retain(VuoOscMessage value);
void VuoOscMessage_release(VuoOscMessage value);
///@}

/**
 * @}
 */
