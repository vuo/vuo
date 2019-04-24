/**
 * @file
 * VuoEventLoop interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <dispatch/dispatch.h>

/**
 * How to execute the event loop.
 */
typedef enum
{
	VuoEventLoop_WaitIndefinitely,
	VuoEventLoop_RunOnce
} VuoEventLoopMode;

void VuoEventLoop_processEvent(VuoEventLoopMode mode);
void VuoEventLoop_break(void);
void VuoEventLoop_switchToAppMode(void);

bool VuoEventLoop_mayBeTerminated(void);

unsigned long VuoEventLoop_getDispatchStrictMask(void);
dispatch_queue_attr_t VuoEventLoop_getDispatchInteractiveAttribute(void);
void VuoEventLoop_installSignalHandlers(void);

void VuoEventLoop_disableAppNap(void);

#ifdef __cplusplus
}
#endif
