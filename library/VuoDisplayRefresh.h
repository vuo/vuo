/**
 * @file
 * VuoDisplayRefresh interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * A state object for firing display refresh events.
 */
typedef void *VuoDisplayRefresh;

VuoDisplayRefresh VuoDisplayRefresh_make(void *context);
void VuoDisplayRefresh_enableTriggers
(
		VuoDisplayRefresh dr,
		void (*requestedFrameTrigger)(VuoReal),
		void (*requestedFrameTriggerWithContext)(VuoReal, void *context)
);
void VuoDisplayRefresh_disableTriggers(VuoDisplayRefresh dr);

#ifdef __cplusplus
}
#endif
