/**
 * @file
 * Logging functions.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLOG_H
#define VUOLOG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup DevelopingNodeClasses DevelopingTypes DevelopingLibraryModules
 * @defgroup VuoModuleDebug Module Debugging
 * Macros to help with debugging.
 *
 * @{
 */

double VuoLogGetTime(void);
double VuoLogGetElapsedTime(void);
bool VuoIsDebugEnabled(void);
char *VuoLog_copyCFDescription(const void *variable);
void VuoLog_backtrace(void);

/**
 * Outputs a message to the system log and to `stderr`.
 *
 * Also stores the most recent 2 messages in the OS X CrashReporter data structure, to be included with crash reports.
 */
#ifdef DOXYGEN
void VuoLog(const char *file, const unsigned int line, const char *function, const char *format, ...);
#else
void VuoLog(const char *file, const unsigned int line, const char *function, const char *format, ...) __attribute__((format(printf, 4, 5)));
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
#define VL() VuoLog(__FILE__, __LINE__, __func__, "")

/**
 * Prints the name of the file and function, and `printf()`-style format/arguments, to the system log and to `stderr` (and implicitly flushes the output buffer).
 *
 * Use this for debugging only; don't commit files with `VLog` calls.  To log messages intended for end users, see @ref VUserLog.
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
 * Same as @ref VLog, but for messages intended for end users.
 *
 * @hideinitializer
 */
#define VUserLog(format, ...) VuoLog(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * Same as @ref VLog, but for debug messages intended for end users.
 *
 * @hideinitializer
 */
#define VDebugLog(format, ...) do { if (VuoIsDebugEnabled()) VuoLog(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__); } while(0)

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

#ifdef PROFILE
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
 * @ingroup DevelopingNodeClasses DevelopingTypes DevelopingLibraryModules
 * @defgroup VuoModuleDefer Defer
 * Macro to emulate Go's (and Swift's) `defer` keyword.
 *
 * @{
 */

#ifndef DOXYGEN
// Executes the block passed to it (an adapter in order to use `__attribute__((cleanup(…)))` with C Blocks).
static inline void VuoDeferCleanup(void (^*b)(void)) { (*b)(); }

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

#endif // VUOLOG_H
