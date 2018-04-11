/**
 * @file
 * VuoRuntimeState implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRuntimeState.hh"

#include <dlfcn.h>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include "VuoCompositionDiff.hh"
#include "VuoEventLoop.h"
#include "VuoHeap.h"
#include "VuoNodeRegistry.hh"
#include "VuoRuntimeCommunicator.hh"
#include "VuoRuntimePersistentState.hh"
#include "VuoThreadManager.hh"

/**
 * Returns a partially initialized runtime instance. @ref VuoRuntimeState::init() should be called to finish the job.
 */
VuoRuntimeState::VuoRuntimeState(void)
{
	persistentState = NULL;
}

/**
 * Cleans up after the composition has stopped for the last time.
 */
VuoRuntimeState::~VuoRuntimeState(void)
{
	delete persistentState;
}

/**
 * Initializes a runtime state instance, updates its references to symbols defined in the composition binary,
 * and opens a connection between it and the runner.
 *
 * @throw std::runtime_error One of the symbols was not found in the composition binary.
 */
void VuoRuntimeState::init(void *zmqContext, const char *controlUrl, const char *telemetryUrl, bool isPaused,
						   pid_t runnerPid, int runnerPipe, bool continueIfRunnerDies, const char *workingDirectory,
						   void *compositionBinaryHandle)
{
	if (! persistentState)
	{
		persistentState = new VuoRuntimePersistentState(workingDirectory);
		persistentState->runtimeState = this;
	}

	this->_isPaused = isPaused;
	hasBeenUnpaused = false;
	_isStopped = false;
	wasStopCompositionCalled = false;
	this->continueIfRunnerDies = continueIfRunnerDies;

	this->runnerPid = runnerPid;

	terminationDisabledCount = 0;
	terminationDisabledQueue = dispatch_queue_create("org.vuo.runtime.terminationDisabled", NULL);

	stopQueue = dispatch_queue_create("org.vuo.runtime.stop", NULL);

	waitForStopTimer = NULL;
	waitForStopCanceledSemaphore = NULL;

	vuoSetup = NULL;
	vuoCleanup = NULL;
	vuoInstanceInit = NULL;
	vuoInstanceFini = NULL;
	vuoInstanceTriggerStart = NULL;
	vuoInstanceTriggerStop = NULL;

	updateCompositionSymbols(compositionBinaryHandle);
	persistentState->communicator->openConnection(zmqContext, controlUrl, telemetryUrl, runnerPipe);
}

/**
 * Cleans up after the composition has stopped for a live-coding reload.
 */
void VuoRuntimeState::fini(void)
{
	vuoMemoryBarrier();
	persistentState->communicator->stopSendingAndCleanUpHeartbeat();
	persistentState->communicator->cleanUpControl();

	if (waitForStopTimer)
	{
		dispatch_source_cancel(waitForStopTimer);
		dispatch_semaphore_wait(waitForStopCanceledSemaphore, DISPATCH_TIME_FOREVER);
		dispatch_release(waitForStopTimer);
		dispatch_release(waitForStopCanceledSemaphore);
	}

	persistentState->communicator->closeConnection();

	dispatch_release(stopQueue);
}

/**
 * Updates references to symbols defined in the composition's generated code.
 *
 * @throw std::runtime_error One of the symbols was not found in the composition binary.
 */
void VuoRuntimeState::updateCompositionSymbols(void *compositionBinaryHandle)
{
	ostringstream errorMessage;

	vuoSetup = (vuoSetupType) dlsym(compositionBinaryHandle, "vuoSetup");
	if (! vuoSetup)
	{
		errorMessage << "The composition couldn't be started because its vuoSetup() function couldn't be found : " << dlerror();
		throw std::runtime_error(errorMessage.str());
	}

	vuoCleanup = (vuoCleanupType) dlsym(compositionBinaryHandle, "vuoCleanup");
	if (! vuoCleanup)
	{
		errorMessage << "The composition couldn't be started because its vuoCleanup() function couldn't be found : " << dlerror();
		throw std::runtime_error(errorMessage.str());
	}

	vuoInstanceInit = (vuoInstanceInitType) dlsym(compositionBinaryHandle, "vuoInstanceInit");
	if (! vuoInstanceInit)
	{
		errorMessage << "The composition couldn't be started because its vuoInstanceInit() function couldn't be found : " << dlerror();
		throw std::runtime_error(errorMessage.str());
	}

	vuoInstanceFini = (vuoInstanceFiniType) dlsym(compositionBinaryHandle, "vuoInstanceFini");
	if (! vuoInstanceFini)
	{
		errorMessage << "The composition couldn't be started because its vuoInstanceFini() function couldn't be found : " << dlerror();
		throw std::runtime_error(errorMessage.str());
	}

	vuoInstanceTriggerStart = (vuoInstanceTriggerStartType) dlsym(compositionBinaryHandle, "vuoInstanceTriggerStart");
	if (! vuoInstanceTriggerStart)
	{
		errorMessage << "The composition couldn't be started because its vuoInstanceTriggerStart() function couldn't be found : " << dlerror();
		throw std::runtime_error(errorMessage.str());
	}

	vuoInstanceTriggerStop = (vuoInstanceTriggerStopType) dlsym(compositionBinaryHandle, "vuoInstanceTriggerStop");
	if (! vuoInstanceTriggerStop)
	{
		errorMessage << "The composition couldn't be started because its vuoInstanceTriggerStop() function couldn't be found : " << dlerror();
		throw std::runtime_error(errorMessage.str());
	}

	persistentState->communicator->updateCompositionSymbols(compositionBinaryHandle);
	persistentState->nodeRegistry->updateCompositionSymbols(compositionBinaryHandle);
}

/**
 * Returns true if the composition is currently paused.
 */
bool VuoRuntimeState::isPaused(void)
{
	return _isPaused;
}

/**
 * Returns true if the composition hasn't started yet or has been stopped.
 */
bool VuoRuntimeState::isStopped(void)
{
	return _isStopped;
}

/**
 * Returns the process ID of the runner that started the composition.
 */
pid_t VuoRuntimeState::getRunnerPid(void)
{
	return runnerPid;
}

/**
 * Returns true if automatic termination is not currently disabled.
 */
bool VuoRuntimeState::mayBeTerminated(void)
{
	__block bool ret;
	dispatch_sync(terminationDisabledQueue, ^{
					  ret = (terminationDisabledCount == 0);
				  });

	return ret;
}

/**
 * Temporarily disables automatic termination.
 *
 * When a composition is asked to shut down, a watchdog timer waits a few seconds then force-quits
 * if it hasn't cleanly shut down by then. This disables that watchdog.
 */
void VuoRuntimeState::disableTermination(void)
{
	dispatch_sync(terminationDisabledQueue, ^{
					  ++terminationDisabledCount;
				  });
}

/**
 * Resumes automatic termination.
 */
void VuoRuntimeState::enableTermination(void)
{
	dispatch_sync(terminationDisabledQueue, ^{
					  --terminationDisabledCount;
					  if (terminationDisabledCount < 0)
						  terminationDisabledCount = 0;
				  });
}

/**
 * Initializes the composition's node and port data, starts the composition's triggers firing
 * if the composition is supposed to be unpaused initially, and starts communicating with the runner.
 */
void VuoRuntimeState::startComposition(void)
{
	persistentState->threadManager->enableSchedulingWorkers();

	vuoSetup();

	if (! _isPaused)
	{
		// Wait to call vuoInstanceInit() until the first time the composition enters an unpaused state.
		// If vuoInstanceInit() were called immediately when the composition is started in a paused state(),
		// then the vuo.event.fireOnStart node's fired event would be ignored.

		hasBeenUnpaused = true;
		__block bool initDone = false;
		dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
						   vuoInstanceInit();
						   vuoInstanceTriggerStart();
						   initDone = true;
						   VuoEventLoop_break();
					   });
		while (! initDone)
			VuoEventLoop_processEvent(VuoEventLoop_WaitIndefinitely);
	}

	persistentState->communicator->startSendingHeartbeat();
	persistentState->communicator->startListeningForControl();

	if (! continueIfRunnerDies)
		persistentState->communicator->startListeningForRunnerExit();
}

/**
 * Pauses firing of the composition's triggers.
 */
void VuoRuntimeState::pauseComposition(void)
{
	_isPaused = true;
	vuoInstanceTriggerStop();
}

/**
 * Resumes firing of the composition's triggers.
 */
void VuoRuntimeState::unpauseComposition(void)
{
	_isPaused = false;

	if (! hasBeenUnpaused)
	{
		hasBeenUnpaused = true;
		vuoInstanceInit();
	}
	vuoInstanceTriggerStart();
}

/**
 * Stops the composition, either for a live-coding reload or permanently, as a result of a
 * stop message from the runner.
 */
void VuoRuntimeState::stopCompositionAsOrderedByRunner(bool isBeingReplaced, int timeoutInSeconds, bool isLastEverInProcess)
{
	if (! isBeingReplaced)
	{
		persistentState->compositionDiff->setDiff(NULL);
	}

	stopComposition(isBeingReplaced, timeoutInSeconds);

	if (timeoutInSeconds >= 0)
	{
		VUOLOG_PROFILE_BEGIN(mainQueue);
		dispatch_sync(dispatch_get_main_queue(), ^{
			VUOLOG_PROFILE_END(mainQueue);
			VuoEventLoop_processEvent(VuoEventLoop_RunOnce);
		});

		VuoCompositionState compositionState = { (void *)this, "" };
		vuoAddCompositionStateToThreadLocalStorage(&compositionState);

		// https://b33p.net/kosada/node/12912
		if (isLastEverInProcess)
		{
			typedef void (*VuoImageTextCacheFiniType)(void);
			VuoImageTextCacheFiniType vuoImageTextCacheFini = (VuoImageTextCacheFiniType) dlsym(RTLD_DEFAULT, "VuoImageTextCache_fini");
			if (vuoImageTextCacheFini)
				vuoImageTextCacheFini();
		}

		VuoHeap_report();

		vuoRemoveCompositionStateFromThreadLocalStorage();
	}
}

/**
 * Nodes/libraries can call this function (via its wrapper, @ref VuoStopComposition)
 * to initiate a clean shutdown of the composition.
 *
 * It's also called if the VuoRunner dies, the composition is still running,
 * and VuoRunner has requested that the composition be stopped when it dies.
 */
void VuoRuntimeState::stopCompositionAsOrderedByComposition(void)
{
	persistentState->setStopRequested(true);

	dispatch_async(persistentState->communicator->getControlQueue(), ^{
					   if (persistentState->communicator->hasZmqConnection())
					   {
						   persistentState->communicator->sendStopRequested();

						   // If we haven't received a response to VuoTelemetryStopRequested within 2 seconds, stop anyway.
						   waitForStopTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, VuoEventLoop_getDispatchStrictMask(), persistentState->communicator->getControlQueue());
						   dispatch_source_set_timer(waitForStopTimer, dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 2.), NSEC_PER_SEC * 2, NSEC_PER_SEC/10);
						   waitForStopCanceledSemaphore = dispatch_semaphore_create(0);

						   dispatch_source_set_event_handler(waitForStopTimer, ^{
							   stopComposition(false, 5);
							   dispatch_source_cancel(waitForStopTimer);
						   });

						   dispatch_source_set_cancel_handler(waitForStopTimer, ^{
							   dispatch_semaphore_signal(waitForStopCanceledSemaphore);
						   });

						   dispatch_resume(waitForStopTimer);
					   }
					   else
					   {
						   stopComposition(false, 5);
					   }
				   });

	persistentState->communicator->interruptListeningForControl();
}

/**
 * This function is called when the composition receives a stop request (@ref VuoControlRequestCompositionStop),
 * or when the a node or library requests a clean shutdown (@ref VuoStopComposition).
 *
 * Stop requests are sent both when @ref VuoRunner::stop is called, and during live-coding reloads
 * (so, in the latter case, the composition process will resume after this function is called).
 *
 * @threadQueue{VuoRuntimeCommunicator::getControlQueue()}
 */
void VuoRuntimeState::stopComposition(bool isBeingReplaced, int timeoutInSeconds)
{
	dispatch_sync(stopQueue, ^{

		// If we're stopping due to a user request, and we've already stopped, don't try to stop again.
		// (The 2-second timer in stopCompositionAsOrderedByComposition() might ding while the previous call is still in progress.)
		if (wasStopCompositionCalled)
			return;
		wasStopCompositionCalled = true;

		killProcessAfterTimeout(timeoutInSeconds);

		dispatch_sync(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
			if (hasBeenUnpaused)
			{
				if (!_isPaused)
				{
					_isPaused = true;
					vuoInstanceTriggerStop();
				}

				vuoInstanceFini();  // Called on a non-main thread to avoid deadlock with VuoRuntimeCommunicator::sendTelemetry().
			}
		});

		if (!isBeingReplaced)
			persistentState->callFiniCallbacks();

		vuoCleanup();

		persistentState->threadManager->disableSchedulingWorkers();

		if (persistentState->communicator->hasZmqConnection())
			persistentState->communicator->stopListeningForControl();
		else
			breakOutOfEventLoop();
	});
}

/**
 * Carries out the final stage of stopping the composition, which entails interrupting the event loop.
 *
 * AFter this function has returned, @ref isStopped() returns true.
 */
void VuoRuntimeState::breakOutOfEventLoop(void)
{
	_isStopped = true;
	VuoEventLoop_break();
}

/**
 * If the composition is not stopped within the timeout, kills this process.
 */
void VuoRuntimeState::killProcessAfterTimeout(int timeoutInSeconds)
{
	if (timeoutInSeconds < 0)
		return;

	if (runnerPid == getpid())
		// If the runner is in the same process as the composition (and possibly other current-process compositions),
		// it would be overkill to kill all of it.
		return;

	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, timeoutInSeconds * NSEC_PER_SEC),
				   dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		if (mayBeTerminated() && VuoEventLoop_mayBeTerminated())
		{
			persistentState->communicator->sendCompositionStoppingAndCloseControl();
			VUserLog("Warning: Waited %d seconds for the composition to cleanly shut down, but it's still running. Now I'm force-quitting it.", timeoutInSeconds);
			kill(getpid(), SIGKILL);
		}
	});
}

extern "C"
{

/**
 * C wrapper for VuoRuntimeState::isPaused().
 */
bool vuoIsPaused(VuoCompositionState *compositionState)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->isPaused();
}

}  // extern "C"
