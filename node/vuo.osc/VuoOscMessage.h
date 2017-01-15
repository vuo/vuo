/**
 * @file
 * VuoOscMessage C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOOSCMESSAGE_H
#define VUOOSCMESSAGE_H

#include "VuoText.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoOscMessage VuoOscMessage
 * An OSC message.
 *
 * @{
 */

/**
 * An OSC message.
 */
typedef struct _VuoOscMessage
{
	VuoText address;
	struct json_object *data;
} *VuoOscMessage;

VuoOscMessage VuoOscMessage_make(VuoText address, struct json_object *data);

VuoOscMessage VuoOscMessage_makeFromJson(struct json_object * js);
struct json_object * VuoOscMessage_getJson(const VuoOscMessage value);
char * VuoOscMessage_getSummary(const VuoOscMessage value);

int VuoOscMessage_getDataCount(const VuoOscMessage value);
struct json_object * VuoOscMessage_getDataJson(const VuoOscMessage value, int index);

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

#endif
