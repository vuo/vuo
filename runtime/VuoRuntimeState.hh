/**
 * @file
 * VuoRuntimeState interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoRuntimePersistentState;
#include "VuoCompositionState.h"

/**
 * Encapsulates the state of the runtime when running a single composition, including both state that is
 * preserved across live-coding reloads and state that only lives for a single start-stop of the composition.
 */
class VuoRuntimeState
{
private:
	bool _isPaused;  ///< True if node execution is currently paused.
	bool hasBeenUnpaused;  ///< True if node execution was unpaused initially, or if it has since been unpaused.
	bool _isStopped; ///< True if composition execution has stopped.
	bool wasStopCompositionCalled;  ///< Prevents the composition from being finalized twice.
	bool continueIfRunnerDies;  ///< If true, the composition continues running if the runner's process ends.

	pid_t runnerPid;  ///< Process ID of the runner that started the composition.

	int terminationDisabledCount;  ///< How many callers have requested that termination be temporarily disabled.
	dispatch_queue_t terminationDisabledQueue;  ///< Synchronizes access to `terminationDisabledCount`.

	dispatch_queue_t stopQueue;	 ///< Ensures stops happen serially.

	dispatch_source_t waitForStopTimer;  ///< Timer for checking if the runner will stop the composition.
	dispatch_semaphore_t waitForStopCanceledSemaphore;  ///< Signaled when no longer checking if the runner will stop the composition.

	/// @{
	/**
	 * Defined in the composition's generated code.
	 */
	typedef void (*vuoSetupType)(void);
	vuoSetupType vuoSetup;
	typedef void (*vuoCleanupType)(void);
	vuoCleanupType vuoCleanup;
	typedef void (*vuoInstanceInitType)(void);
	vuoInstanceInitType vuoInstanceInit;
	typedef void (*vuoInstanceFiniType)(void);
	vuoInstanceFiniType vuoInstanceFini;
	typedef void (*vuoInstanceTriggerStartType)(void);
	vuoInstanceTriggerStartType vuoInstanceTriggerStart;
	typedef void (*vuoInstanceTriggerStopType)(void);
	vuoInstanceTriggerStopType vuoInstanceTriggerStop;
	/// @}

	bool isRunnerInCurrentProcess(void);
	bool mayBeTerminated(void);
	void stopComposition(bool isBeingReplaced, int timeoutInSeconds);
	void killProcessAfterTimeout(int timeoutInSeconds);

public:
	VuoRuntimePersistentState *persistentState;  ///< State preserved across live-coding reloads.

	VuoRuntimeState(void);
	~VuoRuntimeState(void);
	void init(void *zmqContext, const char *controlUrl, const char *telemetryUrl, bool isPaused,
			  pid_t runnerPid, int runnerPipe, bool continueIfRunnerDies, const char *workingDirectory, void *compositionBinaryHandle);
	void fini(void);
	void updateCompositionSymbols(void *compositionBinaryHandle);
	bool isPaused(void);
	bool isStopped(void);
	pid_t getRunnerPid(void);
	void disableTermination(void);
	void enableTermination(void);

	void startComposition(void);
	void pauseComposition(void);
	void unpauseComposition(void);
	void stopCompositionAsOrderedByRunner(bool isBeingReplaced, int timeoutInSeconds);
	void stopCompositionAsOrderedByComposition(void);
	void breakOutOfEventLoop(void);
};

extern "C"
{
bool vuoIsPaused(VuoCompositionState *compositionState);
}
