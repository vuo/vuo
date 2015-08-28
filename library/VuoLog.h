/**
 * @file
 * Logging functions.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLOG_H
#define VUOLOG_H

#include <dispatch/dispatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <asl.h>

/**
 * @ingroup DevelopingNodeClasses DevelopingTypes DevelopingLibraryModules
 * @defgroup VuoModuleDebug Module Debugging
 * Macros to help with debugging.
 *
 * @{
 */


/// @cond
/**
 * Returns the number of seconds (including fractional seconds) since midnight 1970.01.01 GMT.
 */
static double VuoLogGetTime(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec + t.tv_usec / 1000000.;
}

static double VuoLogStartTime;	///< The time when this module was loaded.

/**
 * Stores the time at which this module was loaded, for use by @ref VuoLogGetElapsedTime().
 */
static void __attribute__((constructor)) VuoLogInit(void)
{
	VuoLogStartTime = VuoLogGetTime();
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused"
/**
 * Returns the number of seconds (including fractional seconds) since this module was loaded.
 */
static double VuoLogGetElapsedTime()
{
	return VuoLogGetTime() - VuoLogStartTime;
}

/**
 * Outputs a message to the system log and to `stderr`.
 */
static void VuoLog(const char *file, const unsigned int line, const char *function, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	int size = vsnprintf(NULL, 0, format, args);
	va_end(args);

	char *formattedString = (char *)malloc(size+1);
	va_start(args, format);
	vsnprintf(formattedString, size+1, format, args);
	va_end(args);

	double time = VuoLogGetElapsedTime();

	fprintf(stderr, "\033[38;5;%dm# pid=%d  t=%8.4fs %27s:%-4d  %39s() \t%s\033[0m\n", getpid()%212+19, getpid(), time, file, line, function, formattedString);

	aslmsg msg = asl_new(ASL_TYPE_MSG);
	asl_set(msg, ASL_KEY_READ_UID, "-1");
	asl_log(NULL, msg, ASL_LEVEL_WARNING, "%s:%d  %s()  %s", file, line, function, formattedString);
	asl_free(msg);

	free(formattedString);
}
#pragma clang diagnostic pop
/// @endcond

/**
 * Prints the name of the file and function to the system log and to `stderr` (and implicitly flushes the output buffer).
 *
 * \eg{
 * void nodeEvent()
 * {
 *     VL();
 * }
 * }
 *
 * @hideinitializer
 */
#define VL() VuoLog(__FILE__, __LINE__, __func__, "")

/**
 * Prints the name of the file and function, and `printf()`-style format/arguments, to the system log and to `stderr` (and implicitly flushes the output buffer).
 *
 * \eg{
 * void nodeEvent(VuoInputData(VuoInteger, "42") number)
 * {
 *     VLog("%d", number);
 * }
 * }
 *
 * @hideinitializer
 */
#define VLog(format, ...) VuoLog(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * Prints the name of the current file and function, and the address and description of the specified `heapPointer`, to the system log and to `stderr` (and implicitly flushes the output buffer).
 *
 * \eg{
 * void nodeEvent(VuoInputData(VuoShader) shader)
 * {
 *     VLogHeap(shader);
 * }
 * }
 *
 * @hideinitializer
 */
#define VLogHeap(heapPointer) VLog("%s = %p (registered at %s)", #heapPointer, heapPointer, VuoHeap_getDescription(heapPointer))

/**
 * Prints the specified Core Foundation object to the system log and to `stderr` (and implicitly flushes the output buffer).
 *
 * @hideinitializer
 */
#define VLogCF(coreFoundationRef) { CFStringRef d = CFCopyDescription(coreFoundationRef); CFIndex len = CFStringGetLength(d)+1; char *z = (char *)malloc(len); CFStringGetCString(d, z, len, kCFStringEncodingUTF8); VLog("%s = %s", #coreFoundationRef, z); free(z); CFRelease(d); }

/**
 * @}
 */

#endif // VUOLOG_H
