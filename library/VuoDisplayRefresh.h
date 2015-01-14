/**
 * @file
 * VuoDisplayRefresh interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node.h"

/**
 * A state object for firing display refresh events.
 */
typedef void *VuoDisplayRefresh;

VuoDisplayRefresh VuoDisplayRefresh_make(void);
void VuoDisplayRefresh_enableTriggers
(
		VuoDisplayRefresh dr,
		VuoOutputTrigger(requestedFrame, VuoFrameRequest)
);
void VuoDisplayRefresh_disableTriggers(VuoDisplayRefresh dr);

#ifdef __cplusplus
}
#endif
