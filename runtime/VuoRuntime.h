/**
 * @file
 * VuoRuntime interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoTelemetry.h"

void vuoInit(int argc, char **argv);
void vuoInitInProcess(void *_ZMQContext, const char *controlURL, const char *telemetryURL, bool _isPaused, pid_t _runnerPid);
void vuoTelemetrySend(enum VuoTelemetry type, zmq_msg_t *messages, unsigned int messageCount);
void vuoFini(void);
