/**
 * @file
 * VuoOsc interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node.h"
#include "VuoOscMessage.h"


/**
 * Manages receiving messages via OSC.
 */
typedef void * VuoOscIn;

VuoOscIn VuoOscIn_make(VuoInteger port);
void VuoOscIn_enableTriggers
(
		VuoOscIn oi,
		VuoOutputTrigger(receivedMessage, VuoOscMessage)
);
void VuoOscIn_disableTriggers(VuoOscIn oi);

#ifdef __cplusplus
}
#endif
