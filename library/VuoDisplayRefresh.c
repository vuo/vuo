/**
 * @file
 * VuoDisplayRefresh implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoDisplayRefresh.h"
#include <CoreVideo/CoreVideo.h>

#include "VuoFrameRequest.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoDisplayRefresh",
					 "dependencies" : [
						"ApplicationServices.framework",
						"CoreVideo.framework"
					 ]
				 });
#endif

/**
 * Context data for @c VuoDisplayRefresh_displayLinkCallback.
 */
typedef struct
{
	void (*requestedFrameTrigger)(VuoFrameRequest);	///< The trigger function, to be fired when the display link wants the application to output a frame.
	bool firstRequest;	///< Is this the first time the display link callback has been invoked?
	int64_t initialTime;	///< The output time for which the display link was first invoked.
	VuoInteger frameCount;	///< The nubmer of frames requested so far.

	CVDisplayLinkRef displayLink;	///< The display link, created when @c VuoDisplayRefresh_enableTriggers is called.
} VuoDisplayRefreshInternal;

/**
 * Called by CoreVideo whenever the display link wants the application to output a frame.
 */
static CVReturn VuoDisplayRefresh_displayLinkCallback(CVDisplayLinkRef displayLink,
															const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime,
															CVOptionFlags flagsIn, CVOptionFlags *flagsOut,
															void *ctx)
{
	VuoDisplayRefreshInternal *displayRefresh = (VuoDisplayRefreshInternal *)ctx;
	if (displayRefresh->firstRequest)
	{
		displayRefresh->initialTime = inOutputTime->videoTime;
		displayRefresh->firstRequest = false;
	}

	if (displayRefresh->requestedFrameTrigger)
	{
		double timestamp = (double)(inOutputTime->videoTime - displayRefresh->initialTime)/(double)inOutputTime->videoTimeScale;
		displayRefresh->requestedFrameTrigger(VuoFrameRequest_make(timestamp, displayRefresh->frameCount++));
	}

	return kCVReturnSuccess;
}

/**
 * Creates a display refresh trigger instance.
 *
 * May be called from any thread.
 */
VuoDisplayRefresh VuoDisplayRefresh_make(void)
{
	VuoDisplayRefreshInternal *displayRefresh = (VuoDisplayRefreshInternal *)malloc(sizeof(VuoDisplayRefreshInternal));
	VuoRegister(displayRefresh, free);

	displayRefresh->firstRequest = true;
	// displayRefresh->initialTime is initialized by VuoDisplayRefresh_displayLinkCallback().
	displayRefresh->frameCount = 0;

	return (VuoDisplayRefresh)displayRefresh;
}

/**
 * Starts firing display refresh events.
 *
 * May be called from any thread.
 */
void VuoDisplayRefresh_enableTriggers
(
		VuoDisplayRefresh dr,
		VuoOutputTrigger(requestedFrame, VuoFrameRequest)
)
{
	VuoDisplayRefreshInternal *displayRefresh = (VuoDisplayRefreshInternal *)dr;

	displayRefresh->requestedFrameTrigger = requestedFrame;

	if (CVDisplayLinkCreateWithCGDisplay(kCGDirectMainDisplay, &displayRefresh->displayLink) == kCVReturnSuccess)
	{
		CVDisplayLinkSetOutputCallback(displayRefresh->displayLink, VuoDisplayRefresh_displayLinkCallback, displayRefresh);
		CVDisplayLinkStart(displayRefresh->displayLink);
	}
	else
	{
		VLog("Failed to create CVDisplayLink.");
		displayRefresh->displayLink = NULL;
	}
}

/**
 * After this function returns, this @c VuoDisplayRefresh instance will no longer fire events.
 *
 * May be called from any thread.
 */
void VuoDisplayRefresh_disableTriggers(VuoDisplayRefresh dr)
{
	VuoDisplayRefreshInternal *displayRefresh = (VuoDisplayRefreshInternal *)dr;
	if (displayRefresh->displayLink)
		CVDisplayLinkStop(displayRefresh->displayLink);
}
