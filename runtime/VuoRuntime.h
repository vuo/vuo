/**
 * @file
 * VuoRuntime interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoCompositionState.h"

void vuoInit(int argc, char **argv);

/**
 * Type for @ref vuoInitInProcess.
 */
typedef void (VuoInitInProcessType)(void *ZMQContext, const char *controlURL, const char *telemetryURL, bool isPaused, pid_t runnerPid,
									int runnerPipe, bool continueIfRunnerDies, bool trialRestrictionsEnabled, const char *workingDirectory,
									void *compositionBinaryHandle, void *runtimePersistentState);

void vuoInitInProcess(void *ZMQContext, const char *controlURL, const char *telemetryURL, bool isPaused, pid_t runnerPid,
					  int runnerPipe, bool continueIfRunnerDies, bool trialRestrictionsEnabled, const char *workingDirectory,
					  void *compositionBinaryHandle, void *runtimePersistentState);

/**
 * Type for @ref vuoFini.
 */
typedef void * (VuoFiniType)(void);

void * vuoFini(void);

/**
 * Type for @ref vuoFiniRuntimePersistentState.
 */
typedef void (VuoFiniRuntimePersistentStateType)(void *);

/**
 * Type for @ref vuoSendError.
 */
typedef void (VuoSendErrorType)(struct VuoCompositionState *, const char *);

/**
 * Sends a telemetry message indicating that an error has occurred.
 */
VuoSendErrorType vuoSendError;

/**
 * Type for @ref vuoIsCurrentCompositionStopped.
 */
typedef bool (VuoIsCurrentCompositionStoppedType)(void);

/**
 * Returns true if the composition has not yet started or if it has stopped.
 *
 * Assumes that just one composition is running in the process.
 */
VuoIsCurrentCompositionStoppedType vuoIsCurrentCompositionStopped;

#ifdef __cplusplus
}
#endif
