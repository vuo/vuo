/**
 * @file
 * VuoHeap implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoHeap.h"
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <map>
#include <set>
#include <sstream>
#include <iomanip>
using namespace std;
#include "VuoLog.h"

//#define VUOHEAP_TRACE		// NOCOMMIT
//#define VUOHEAP_TRACEALL	// NOCOMMIT
static set<const void *> *VuoHeap_trace;	///< Heap pointers to trace.

/**
 * Calls the sendError() function defined in VuoRuntime (without introducing a direct dependency on VuoRuntime).
 */
void sendErrorWrapper(const char *message)
{
#ifdef VUOHEAP_TRACE
	fprintf(stderr, "%s\n", message);
	VuoLog_backtrace();
#endif

	typedef void (*sendErrorType)(const char *message);
	sendErrorType sendError = (sendErrorType) dlsym(RTLD_SELF, "sendError");  // for running composition in separate process as executable or in current process
	if (! sendError)
		sendError = (sendErrorType) dlsym(RTLD_DEFAULT, "sendError");  // for running composition in separate process as dynamic libraries
	if (sendError)
		sendError(message);
	else
		VUserLog("%s", message);
}

/**
 * An entry in the reference-counting table.
 */
typedef struct
{
	int referenceCount;
	DeallocateFunctionType deallocateFunction;

	const char *file;
	unsigned int line;
	const char *function;
	const char *variable;
} VuoHeapEntry;

static map<const void *, VuoHeapEntry> *referenceCounts;  ///< The reference count for each pointer.
static set<const void *> *singletons;  ///< Known singleton pointers.
static dispatch_semaphore_t referenceCountsSemaphore = NULL;  ///< Synchronizes access to referenceCounts.

/**
 * Copies the first 16 printable characters of `pointer` into `summary`, followed by char 0.
 */
static void VuoHeap_makeSafePointerSummary(char *summary, const void *pointer)
{
	char *pointerAsChar = (char *)pointer;
	for (int i = 0; i < 16; ++i)
		if (isprint(pointerAsChar[i]))
			summary[i] = pointerAsChar[i];
		else
			summary[i] = '_';
	summary[16] = 0;
}

/**
 * Copies the first 16 printable characters of `pointer` into `summary`, followed by char 0.
 */
static char *VuoHeap_makeDescription(VuoHeapEntry e)
{
	const char *format = "%s:%d :: %s() :: %s";
	int size = snprintf(NULL, 0, format,  e.file, e.line, e.function, e.variable);
	char *description = (char *)malloc(size+1);
	snprintf(description, size+1, format, e.file, e.line, e.function, e.variable);
	return description;
}

/**
 * Initializes the reference-counting system.
 *
 * This function is automatically added near the beginning of the linker module's call graph.
 */
static void __attribute__((constructor(101))) VuoHeap_init()
{
	referenceCounts = new map<const void *, VuoHeapEntry>;
	singletons = new set<const void *>;
	VuoHeap_trace = new set<const void *>;
	referenceCountsSemaphore = dispatch_semaphore_create(1);

#if 0
	// Periodically dump the referenceCounts table, to help find leaks.
	const double dumpInterval = 5.0; // seconds
	dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
	dispatch_source_set_timer(timer, dispatch_walltime(NULL,0), NSEC_PER_SEC*dumpInterval, NSEC_PER_SEC*dumpInterval);
	dispatch_source_set_event_handler(timer, ^{
										  fprintf(stderr, "\n\n\n\n\nreferenceCounts:\n");
										  dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
										  for (map<const void *, VuoHeapEntry>::iterator i = referenceCounts->begin(); i != referenceCounts->end(); ++i)
										  {
											  const void *heapPointer = i->first;
											  char pointerSummary[17];
											  VuoHeap_makeSafePointerSummary(pointerSummary, heapPointer);
											  char *description = VuoHeap_makeDescription(i->second);
											  fprintf(stderr, "\t% 3d refs to %p \"%s\", registered at %s\n", i->second.referenceCount, heapPointer, pointerSummary, description);
											  free(description);
										  }
										  dispatch_semaphore_signal(referenceCountsSemaphore);
									  });
	dispatch_resume(timer);
#endif
}

/**
 * Sends a telemetry error with information about any objects remaining in the reference counting table.
 */
void VuoHeap_report(void)
{
	VUOLOG_PROFILE_BEGIN(referenceCountsSemaphore);
	dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
	VUOLOG_PROFILE_END(referenceCountsSemaphore);
	{

	if (! referenceCounts->empty())
	{
		ostringstream errorMessage;
		errorMessage << "VuoRelease was not called enough times for:" << endl;
		for (map<const void *, VuoHeapEntry>::iterator i = referenceCounts->begin(); i != referenceCounts->end(); ++i)
		{
			const void *heapPointer = i->first;
			char pointerSummary[17];
			VuoHeap_makeSafePointerSummary(pointerSummary, heapPointer);
			char *description = VuoHeap_makeDescription(i->second);
			errorMessage << "\t" << setw(3) << i->second.referenceCount << " refs to " << heapPointer << " \"" << pointerSummary << "\", registered at " << description << endl;
			free(description);
		}
		sendErrorWrapper(errorMessage.str().c_str());
	}

	}
	dispatch_semaphore_signal(referenceCountsSemaphore);
}

/**
 * Instead of this function, you probably want to use VuoRegister(). This function is used to implement
 * the VuoRegister() macro.
 *
 * Registers @a heapPointer to be reference-counted and stores its deallocate function
 * (unless @a heapPointer is null or is already being reference-counted).
 *
 * @param heapPointer A pointer to allocated memory on the heap.
 * @param deallocate The function to be used to deallocate the memory when the reference count gets back to its original value of 0.
 * @param file The name of the file in which VuoRegister() is called.
 * @param line The line in the file in which VuoRegister() is called.
 * @param func The function in which VuoRegister() is called.
 * @param pointerName The stringified pointer variable name.
 * @return The updated reference count of @a heapPointer. This is 0 if @a heapPointer is not already being reference-counted, greater than 0 if it is, or -1 if @a heapPointer is null.
 */
int VuoRegisterF(const void *heapPointer, DeallocateFunctionType deallocate, const char *file, unsigned int line, const char *func, const char *pointerName)
{
	if (! heapPointer)
		return -1;

	bool isAlreadyReferenceCounted;
	int updatedCount;

	VUOLOG_PROFILE_BEGIN(referenceCountsSemaphore);
	dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
	VUOLOG_PROFILE_END(referenceCountsSemaphore);
	{

#ifdef VUOHEAP_TRACE
#ifndef VUOHEAP_TRACEALL
		if (VuoHeap_trace->find(heapPointer) != VuoHeap_trace->end())
#endif
		{
			fprintf(stderr, "VuoRegister(%p)  %s\n", heapPointer, pointerName);
			VuoLog_backtrace();
		}
#endif

		map<const void *, VuoHeapEntry>::iterator i = referenceCounts->find(heapPointer);
		isAlreadyReferenceCounted = (i != referenceCounts->end());
		if (! isAlreadyReferenceCounted)
		{
			updatedCount = 0;
			(*referenceCounts)[heapPointer] = (VuoHeapEntry){updatedCount, deallocate, file, line, func, pointerName};
		}
		else
			updatedCount = i->second.referenceCount;

	}
	dispatch_semaphore_signal(referenceCountsSemaphore);

	if (isAlreadyReferenceCounted)
	{
		ostringstream errorMessage;
		char *description = VuoHeap_makeDescription((VuoHeapEntry){0, NULL, file, line, func, pointerName});
		errorMessage << "VuoRegister was called more than once for " << heapPointer << " " << description;
		free(description);
		sendErrorWrapper(errorMessage.str().c_str());
	}

	return updatedCount;
}

/**
 * Instead of this function, you probably want to use VuoRegisterSingleton(). This function is used to implement
 * the VuoRegisterSingleton() macro.
 */
int VuoRegisterSingletonF(const void *heapPointer, const char *file, unsigned int line, const char *func, const char *pointerName)
{
	if (! heapPointer)
		return -1;

	bool isAlreadyReferenceCounted;

	VUOLOG_PROFILE_BEGIN(referenceCountsSemaphore);
	dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
	VUOLOG_PROFILE_END(referenceCountsSemaphore);
	{

#ifdef VUOHEAP_TRACE
#ifndef VUOHEAP_TRACEALL
		if (VuoHeap_trace->find(heapPointer) != VuoHeap_trace->end())
#endif
		{
			fprintf(stderr, "VuoRegisterSingleton(%p)  %s\n", heapPointer, pointerName);
			VuoLog_backtrace();
		}
#endif

		// Remove the singleton from the main reference-counting table, if it exists there.
		// Enables reclassifying a pointer that was already VuoRegister()ed.
		referenceCounts->erase(heapPointer);

		// Add the singleton to the singleton table.
		isAlreadyReferenceCounted = (singletons->find(heapPointer) != singletons->end());
		if (! isAlreadyReferenceCounted)
			singletons->insert(heapPointer);
	}
	dispatch_semaphore_signal(referenceCountsSemaphore);

	if (isAlreadyReferenceCounted)
	{
		ostringstream errorMessage;
		char *description = VuoHeap_makeDescription((VuoHeapEntry){0, NULL, file, line, func, pointerName});
		errorMessage << "VuoRegisterSingleton was called more than once for " << heapPointer << " " << description;
		free(description);
		sendErrorWrapper(errorMessage.str().c_str());
	}

	return isAlreadyReferenceCounted ? 1 : 0;
}

/**
 * Wrapper for @ref VuoRetain.
 *
 * This used to be an instrumented function, but the instrumentation was removed in Vuo 1.2 to improve performance.
 * It's still here so that plugins compiled with Vuo 0.9 through 1.1 have a better chance of linking successfully.
 *
 * @deprecated Use @ref VuoRetain instead.
 */
extern "C" int VuoRetainF(const void *heapPointer, const char *file, unsigned int line, const char *func)
{
	return VuoRetain(heapPointer);
}

/**
 * @ingroup ReferenceCountingFunctions
 * Increments the reference count for @a heapPointer (unless @a heapPointer is not being reference-counted).
 *
 * @param heapPointer A pointer to allocated memory on the heap.
 * @return The updated reference count of @a heapPointer, or -1 if @a heapPointer is not being reference-counted or is null.
 */
int VuoRetain(const void *heapPointer)
{
	if (! heapPointer)
		return -1;

	int updatedCount = -1;
	bool foundSingleton = false;
	VuoHeapEntry entry;

	VUOLOG_PROFILE_BEGIN(referenceCountsSemaphore);
	dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
	VUOLOG_PROFILE_END(referenceCountsSemaphore);
	{
		map<const void *, VuoHeapEntry>::iterator i = referenceCounts->find(heapPointer);

#ifdef VUOHEAP_TRACE
#ifndef VUOHEAP_TRACEALL
		if (VuoHeap_trace->find(heapPointer) != VuoHeap_trace->end())
#endif
		{
			fprintf(stderr, "VuoRetain(%p)  %s\n", heapPointer, (i != referenceCounts->end()) ? i->second.variable : "");
			VuoLog_backtrace();
		}
#endif

		if (i != referenceCounts->end())
			updatedCount = ++(i->second.referenceCount);
		else
			foundSingleton = singletons->find(heapPointer) != singletons->end();
		entry = i->second;

	}
	dispatch_semaphore_signal(referenceCountsSemaphore);

	if (updatedCount == -1 && !foundSingleton)
	{
		ostringstream errorMessage;
		errorMessage << "VuoRetain was called for unregistered pointer " << heapPointer;
		sendErrorWrapper(errorMessage.str().c_str());
	}

	return updatedCount;
}

/**
 * Wrapper for @ref VuoRelease.
 *
 * This used to be an instrumented function, but the instrumentation was removed in Vuo 1.2 to improve performance.
 * It's still here so that plugins compiled with Vuo 0.9 through 1.1 have a better chance of linking successfully.
 *
 * @deprecated Use @ref VuoRelease instead.
 */
extern "C" int VuoReleaseF(const void *heapPointer, const char *file, unsigned int line, const char *func)
{
	return VuoRelease(heapPointer);
}

/**
 * @ingroup ReferenceCountingFunctions
 * Decrements the reference count for @a heapPointer (unless @a heapPointer is not being reference-counted).
 * If the reference count becomes 0, @a heapPointer is deallocated and is no longer reference-counted.
 *
 * @param heapPointer A pointer to allocated memory on the heap.
 * @return The updated reference count of @a heapPointer, or -1 if @a heapPointer is not being reference-counted, has never been retained, or is null.
 */
int VuoRelease(const void *heapPointer)
{
	if (! heapPointer)
		return -1;

	int updatedCount = -1;
	bool foundSingleton = false;
	bool isRegisteredWithoutRetain = false;
	DeallocateFunctionType deallocate = NULL;

	VUOLOG_PROFILE_BEGIN(referenceCountsSemaphore);
	dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
	VUOLOG_PROFILE_END(referenceCountsSemaphore);
	{
		map<const void *, VuoHeapEntry>::iterator i = referenceCounts->find(heapPointer);

#ifdef VUOHEAP_TRACE
#ifndef VUOHEAP_TRACEALL
		if (VuoHeap_trace->find(heapPointer) != VuoHeap_trace->end())
#endif
		{
			fprintf(stderr, "VuoRelease(%p)  %s\n", heapPointer, (i != referenceCounts->end()) ? i->second.variable : "");
			VuoLog_backtrace();
		}
#endif

		if (i != referenceCounts->end())
		{
			if (i->second.referenceCount == 0)
			{
				isRegisteredWithoutRetain = true;
			}
			else
			{
				updatedCount = --(i->second.referenceCount);

				if (updatedCount == 0)
				{
					deallocate = i->second.deallocateFunction;
					referenceCounts->erase(heapPointer);
				}
			}
		}
		else
			foundSingleton = singletons->find(heapPointer) != singletons->end();

	}
	dispatch_semaphore_signal(referenceCountsSemaphore);

	if (updatedCount == 0)
	{
#ifdef VUOHEAP_TRACE
#ifndef VUOHEAP_TRACEALL
		if (VuoHeap_trace->find(heapPointer) != VuoHeap_trace->end())
#endif
		{
			fprintf(stderr, "VuoDeallocate(%p)\n", heapPointer);
//			VuoLog_backtrace();
		}
#endif

		deallocate((void *)heapPointer);
	}
	else if (updatedCount == -1 && !foundSingleton)
	{
		ostringstream errorMessage;
		errorMessage << "VuoRelease was called for "
					 << (isRegisteredWithoutRetain ? "unretained" : "unregistered")
					 << " pointer " << heapPointer;
		sendErrorWrapper(errorMessage.str().c_str());
	}

	return updatedCount;
}

/**
 * Returns a description of the specified @a heapPointer:
 * the file, line, and function where VuoRegister() was called,
 * and the variable name.
 *
 * The caller is responsible for freeing the returned string.
 */
const char * VuoHeap_getDescription(const void *heapPointer)
{
	map<const void *, VuoHeapEntry>::iterator i = referenceCounts->find(heapPointer);
	if (i != referenceCounts->end())
		return VuoHeap_makeDescription(i->second);

	return strdup("(pointer was not VuoRegister()ed)");
}

/**
 * Pass a pointer to this function to log all its subsequent retains and releases.
 *
 * This only has any effect when preprocessor macro `VUOHEAP_TRACE` is defined in `VuoHeap.cc`.
 */
void VuoHeap_addTrace(const void *heapPointer)
{
	VUOLOG_PROFILE_BEGIN(referenceCountsSemaphore);
	dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
	VUOLOG_PROFILE_END(referenceCountsSemaphore);
	{
		VuoHeap_trace->insert(heapPointer);
	}
	dispatch_semaphore_signal(referenceCountsSemaphore);
}
