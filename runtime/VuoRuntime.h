/**
 * @file
 * VuoRuntime interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoTelemetry.h"

#ifdef __cplusplus
extern "C"
{
#endif

void vuoInit(int argc, char **argv);

/**
 * Type for @ref vuoInitInProcess.
 */
typedef void (VuoInitInProcessType)(void *_ZMQContext, const char *controlURL, const char *telemetryURL, bool _isPaused, pid_t _runnerPid,
					  int runnerPipe, bool continueIfRunnerDies, bool trialRestrictionsEnabled, void *VuoCompositionFiniCallbackList);

/**
 * Sets up ZMQ control and telemetry sockets, then calls the generated function @c setup().
 * If the composition is not paused, also calls @c nodeInstanceInit() and @c nodeInstanceTriggerStart().
 */
VuoInitInProcessType vuoInitInProcess;

void vuoTelemetrySend(enum VuoTelemetry type, zmq_msg_t *messages, unsigned int messageCount);

/**
 * Type for @ref vuoFini.
 */
typedef void (VuoFiniType)(void);

/**
 * Cleans up composition execution: closes the ZMQ sockets and dispatch source and queues.
 * Assumes the composition has received and replied to a @c VuoControlRequestCompositionStop message.
 */
VuoFiniType vuoFini;

#ifdef __cplusplus
}
#endif
