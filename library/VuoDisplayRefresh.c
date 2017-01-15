/**
 * @file
 * VuoDisplayRefresh implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoDisplayRefresh.h"
#include <CoreVideo/CoreVideo.h>
#include <mach/mach_time.h>

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
	void (*requestedFrameTrigger)(VuoReal);	///< The trigger function to be fired when the display link wants the application to output a frame.  May be @c NULL.
	void (*requestedFrameTriggerWithContext)(VuoReal, void *context);	///< The trigger function to be fired when the display link wants the application to output a frame.  May be @c NULL.
	void *triggerContext;	///< Custom contextual data to be passed to @c requestedFrameTriggerWithContext.

	bool firstRequest;	///< Is this the first time the display link callback has been invoked?
	int64_t initialTime;	///< The output time for which the display link was first invoked.
	VuoInteger frameCount;	///< The nubmer of frames requested so far.

	CVDisplayLinkRef displayLink;	///< The display link, created when @c VuoDisplayRefresh_enableTriggers is called.
	bool displayLinkCancelRequested;	///< True if the displayLink should cancel next refresh.
	dispatch_semaphore_t displayLinkCanceledAndCompleted;	///< Signaled after the last displayLinkCallback has completed during cancellation.
} VuoDisplayRefreshInternal;

/**
 * Called by CoreVideo whenever the display link wants the application to output a frame.
 */
static CVReturn VuoDisplayRefresh_displayLinkCallback(CVDisplayLinkRef displayLink,
															const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime,
															CVOptionFlags flagsIn, CVOptionFlags *flagsOut,
															void *ctx)
{
	int64_t plannedFrameTime = inOutputTime->videoTime;

	VuoDisplayRefreshInternal *displayRefresh = (VuoDisplayRefreshInternal *)ctx;
	if (displayRefresh->firstRequest)
	{
		displayRefresh->initialTime = plannedFrameTime;
		displayRefresh->firstRequest = false;
	}

	if (displayRefresh->displayLinkCancelRequested)
	{
		CVDisplayLinkStop(displayRefresh->displayLink);
		dispatch_semaphore_signal(displayRefresh->displayLinkCanceledAndCompleted);
		return kCVReturnSuccess;
	}

	VuoReal timestamp = (double)(plannedFrameTime - displayRefresh->initialTime)/(double)inOutputTime->videoTimeScale;

	if (displayRefresh->requestedFrameTrigger)
		displayRefresh->requestedFrameTrigger(timestamp);

	if (displayRefresh->requestedFrameTriggerWithContext)
		displayRefresh->requestedFrameTriggerWithContext(timestamp, displayRefresh->triggerContext);

	return kCVReturnSuccess;
}

/**
 * Creates a display refresh trigger instance.
 *
 * @threadAny
 *
 * @param context Specifies custom contextual data to be passed to the @c requestedFrameSendoff callback (see @ref VuoDisplayRefreshInternal)
 */
VuoDisplayRefresh VuoDisplayRefresh_make(void *context)
{
	VuoDisplayRefreshInternal *displayRefresh = (VuoDisplayRefreshInternal *)malloc(sizeof(VuoDisplayRefreshInternal));
	VuoRegister(displayRefresh, free);

	displayRefresh->firstRequest = true;
	// displayRefresh->initialTime is initialized by VuoDisplayRefresh_displayLinkCallback().
	displayRefresh->frameCount = 0;

	displayRefresh->triggerContext = context;

	return (VuoDisplayRefresh)displayRefresh;
}

/**
 * Starts firing display refresh events.
 *
 * @see VuoDisplayRefreshInternal
 *
 * @threadAny
 */
void VuoDisplayRefresh_enableTriggers
(
		VuoDisplayRefresh dr,
		void (*requestedFrameTrigger)(VuoReal),
		void (*requestedFrameTriggerWithContext)(VuoReal, void *context)
)
{
	VuoDisplayRefreshInternal *displayRefresh = (VuoDisplayRefreshInternal *)dr;

	displayRefresh->requestedFrameTrigger = requestedFrameTrigger;
	displayRefresh->requestedFrameTriggerWithContext = requestedFrameTriggerWithContext;

	if (CVDisplayLinkCreateWithCGDisplay(kCGDirectMainDisplay, &displayRefresh->displayLink) == kCVReturnSuccess)
	{
		displayRefresh->displayLinkCancelRequested = false;
		displayRefresh->displayLinkCanceledAndCompleted = dispatch_semaphore_create(0);
		CVDisplayLinkSetOutputCallback(displayRefresh->displayLink, VuoDisplayRefresh_displayLinkCallback, displayRefresh);
		CVDisplayLinkStart(displayRefresh->displayLink);

		CVTime nominal = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(displayRefresh->displayLink);
		VDebugLog("Refresh: %g Hz (%d/%lld)", (double)nominal.timeScale/nominal.timeValue, nominal.timeScale, nominal.timeValue);

		CVTime latency = CVDisplayLinkGetOutputVideoLatency(displayRefresh->displayLink);
		if (latency.timeScale)
		{
			double latencyFrames = ((double)latency.timeValue/latency.timeScale)/((double)nominal.timeValue/nominal.timeScale);
			VDebugLog("Latency: %g frame%s (%d/%lld)", latencyFrames, fabs(latencyFrames-1)<0.00001 ? "" : "s", latency.timeScale, latency.timeValue);
		}
		else
			VDebugLog("Latency: unknown");
	}
	else
	{
		VUserLog("Failed to create CVDisplayLink.");
		displayRefresh->displayLink = NULL;
	}
}

/**
 * After this function returns, this @c VuoDisplayRefresh instance will no longer fire events.
 *
 * @threadAny
 */
void VuoDisplayRefresh_disableTriggers(VuoDisplayRefresh dr)
{
	VuoDisplayRefreshInternal *displayRefresh = (VuoDisplayRefreshInternal *)dr;
	if (displayRefresh->displayLink)
	{
		displayRefresh->displayLinkCancelRequested = true;

		// Wait for the last sendoff to complete.
		dispatch_semaphore_wait(displayRefresh->displayLinkCanceledAndCompleted, DISPATCH_TIME_FOREVER);
	}
}
