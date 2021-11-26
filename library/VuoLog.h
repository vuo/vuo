/**
 * @file
 * Logging functions.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

// Don't use `#pragma once` here, since this header is included both from the source tree and from Vuo.framework.
#ifndef VUO_LOG
#define VUO_LOG

#ifdef __cplusplus
extern "C" {
#endif

#include <dlfcn.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * @ingroup DevelopingNodeClasses
 * @defgroup VuoModuleDebug Module Debugging
 * Macros to help with debugging.
 *
 * @{
 */

double VuoLogGetTime(void);
double VuoLogGetElapsedTime(void);
bool VuoIsDebugEnabled(void);

void VuoLog_backtrace(void);
#ifdef __cplusplus
}
#include <vector>
std::vector<std::string> VuoLog_getBacktrace(void);
extern "C" {
#endif

bool VuoLog_isDebuggerAttached(void);

static char *VuoLog_moduleName = 0;  ///< The name of the current dylib (as opposed to the process's main executable), set by `VuoLog_initModuleName`.

/**
 * Gets the name of the dylib the log function was called from, to help differentiate it
 * from other Vuo dylibs (e.g. separate exported FFGL plugins) in the same process.
 */
static void __attribute__((constructor(90))) VuoLog_initModuleName(void)
{
	Dl_info info;
	if (dladdr((void *)VuoLog_initModuleName, &info) && info.dli_fname)
	{
		VuoLog_moduleName = basename_r(info.dli_fname, (char *)malloc(strlen(info.dli_fname + 1)));

		if (VuoLog_moduleName)
		{
			// Trim off the `lib` prefix, if any.
			if (strncmp(VuoLog_moduleName, "lib", 3) == 0)
				VuoLog_moduleName += 3;

			// Trim off the extension, if any.
			char *dot = strrchr(VuoLog_moduleName, '.');
			if (dot)
				*dot = 0;
		}
	}
}

/**
 * Stores status information so it can be included in crash reports.
 * Only the most recent status is stored.
 * Use `NULL` to clear the status.
 *
 * Similar to VUserLog()/VDebugLog(),
 * but useful for highly verbose status messages where only the most recent is relevant.
 *
 * @hideinitializer
 */
#define VuoLog_status(format, ...) VuoLog_statusF(VuoLog_moduleName, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * Helper for VuoLog_status.
 */
#ifdef DOXYGEN
void VuoLog_statusF(const char *moduleName, const char *file, const unsigned int linenumber, const char *function, const char *format, ...);
#else
void VuoLog_statusF(const char *moduleName, const char *file, const unsigned int linenumber, const char *function, const char *format, ...) __attribute__((format(printf, 5, 6)));
#endif

/**
 * Outputs a message to the system log and to `stderr`.
 *
 * Also stores the most recent several messages in the OS X CrashReporter data structure, to be included with crash reports.
 */
#ifdef DOXYGEN
void VuoLog(const char *moduleName, const char *file, const unsigned int linenumber, const char *function, const char *format, ...);
#else
void VuoLog(const char *moduleName, const char *file, const unsigned int linenumber, const char *function, const char *format, ...) __attribute__((format(printf, 5, 6)));
#endif

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
#define VL() VuoLog(VuoLog_moduleName, __FILE__, __LINE__, __func__, "")

/**
 * Prints the name of the file and function, and `printf()`-style format/arguments, to the system log and to `stderr` (and implicitly flushes the output buffer).
 *
 * Use this for debugging only; don't commit files with `VLog` calls.  To log messages intended for end users, see @ref VUserLog.
 *
 * \eg{
 * void nodeEvent(VuoInputData(VuoInteger, "42") number)
 * {
 *     VLog("%lld", number);
 * }
 * }
 *
 * @hideinitializer
 */
#define VLog(format, ...) VuoLog(VuoLog_moduleName, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * Same as @ref VLog, but for messages intended for end users.
 *
 * @hideinitializer
 */
#define VUserLog(format, ...) VuoLog(VuoLog_moduleName, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * Same as @ref VLog, but for debug messages intended for end users.
 *
 * @hideinitializer
 */
#define VDebugLog(format, ...) do { if (VuoIsDebugEnabled()) VuoLog(VuoLog_moduleName, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); } while(0)

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
#define VLogHeap(heapPointer) do { \
	char *description = VuoHeap_getDescription(heapPointer); \
	VLog("%s = %p (registered at %s)", #heapPointer, heapPointer, description); \
	free(description); \
	} while(0)


void VuoLog_recordTime(const char *name, double time);

#ifdef VUO_PROFILE
#define VUOLOG_PROFILE_BEGIN(object) double VuoLog_time_ ## object = VuoLogGetTime();
#define VUOLOG_PROFILE_END(object) VuoLog_recordTime(#object, VuoLogGetTime() - VuoLog_time_ ## object);
#else
/**
 * Starts a profiler time sample.
 *
 * In this macro's parameter, pass a text identifier for the time sample (such as the variable name of a libdispatch queue or semaphore).
 *
 * \eg{
 * VUOLOG_PROFILE_BEGIN(referenceCountsSemaphore);
 * dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
 * VUOLOG_PROFILE_END(referenceCountsSemaphore);
 * }
 */
#define VUOLOG_PROFILE_BEGIN(object)

/**
 * Ends a profiler time sample.
 */
#define VUOLOG_PROFILE_END(object)
#endif

/**
 * @}
 */

/**
 * @ingroup DevelopingNodeClasses
 * @defgroup VuoModuleDefer Defer
 * Macro to emulate Go's (and Swift's) `defer` keyword.
 *
 * @{
 */

#ifndef DOXYGEN
// Executes the block passed to it (an adapter in order to use `__attribute__((cleanup(…)))` with C Blocks).
static inline void VuoDeferCleanup(void (^*b)(void)) { if (*b) (*b)(); }

// Combines two strings into an identifier.
#define VuoDeferMerge(a,b) a##b

// Create the name of the defer scope variable.
#define VuoDeferVarName(a) VuoDeferMerge(VuoDeferScopeVar, a)
#endif

/**
 * Defers execution of the specified block until the end of scope.
 *
 * \eg{
 * FILE *a = fopen("a.txt", "r");
 * if (!a)
 *     return;
 * VuoDefer ^{ fclose(a); };
 * }
 *
 * See also http://fdiv.net/2015/10/08/emulating-defer-c-clang-or-gccblocks
 *
 * @hideinitializer
 */
#define VuoDefer __attribute__((cleanup(VuoDeferCleanup),unused)) void (^VuoDeferVarName(__COUNTER__))(void) =

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
