/**
 * @file
 * VuoFrameRequest C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOFRAMEREQUEST_H
#define VUOFRAMEREQUEST_H

#include "VuoReal.h"
#include "VuoInteger.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoFrameRequest VuoFrameRequest
 * Information about a request for rendering a new graphical frame.
 *
 * @{
 */

/**
 * Information about a request for rendering a new graphical frame.
 */
typedef struct
{
	VuoReal timestamp;
	VuoInteger frameCount;

	char blah[42]; ///< @todo https://b33p.net/kosada/node/4124
} VuoFrameRequest;

VuoFrameRequest VuoFrameRequest_valueFromJson(struct json_object * js);
struct json_object * VuoFrameRequest_jsonFromValue(const VuoFrameRequest value);
char * VuoFrameRequest_summaryFromValue(const VuoFrameRequest value);

/// @{
/**
 * Automatically generated function.
 */
VuoFrameRequest VuoFrameRequest_valueFromString(const char *str);
char * VuoFrameRequest_stringFromValue(const VuoFrameRequest value);
/// @}

/**
 * Returns a point with the specified coordinates.
 */
static inline VuoFrameRequest VuoFrameRequest_make(VuoReal timestamp, VuoInteger frameCount) __attribute__((const));
static inline VuoFrameRequest VuoFrameRequest_make(VuoReal timestamp, VuoInteger frameCount)
{
	VuoFrameRequest fr;
	fr.timestamp = timestamp;
	fr.frameCount = frameCount;
	return fr;
}

/**
 * @}
 */

#endif
