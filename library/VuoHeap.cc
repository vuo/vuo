/**
 * @file
 * VuoHeap implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoHeap.h"
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <map>
#include <set>
#include <sstream>
#include <iomanip>
using namespace std;
#include "VuoLog.h"
#include "VuoRuntime.h"

static set<const void *> *VuoHeap_trace;	///< Heap pointers to trace.
static pthread_mutex_t VuoHeap_mutex = PTHREAD_MUTEX_INITIALIZER;  ///< Protects access to `referenceCounts`.

/**
 * Calls the vuoSendError() function defined in the runtime (without introducing a direct dependency on the runtime).
 */
void sendErrorWrapper(const char *message)
{
#ifdef VUOHEAP_TRACE
	fprintf(stderr, "%s\n", message);
	VuoLog_backtrace();
#endif

	VuoSendErrorType *vuoSendError = (VuoSendErrorType *) dlsym(RTLD_SELF, "vuoSendError");  // for running composition in separate process as executable or in current process
	if (! vuoSendError)
		vuoSendError = (VuoSendErrorType *) dlsym(RTLD_DEFAULT, "vuoSendError");  // for running composition in separate process as dynamic libraries

	pthread_key_t *vuoCompositionStateKey = (pthread_key_t *) dlsym(RTLD_SELF, "vuoCompositionStateKey");
	if (! vuoCompositionStateKey)
		vuoCompositionStateKey = (pthread_key_t *) dlsym(RTLD_DEFAULT, "vuoCompositionStateKey");

	void *compositionState = NULL;
	if (vuoCompositionStateKey)
		compositionState = pthread_getspecific(*vuoCompositionStateKey);

	if (vuoSendError && compositionState)
		vuoSendError((VuoCompositionState *)compositionState, message);
	else
		VUserLog("%s", message);
}

#ifdef VUOHEAP_TRACEALL
/**
 * Returns true if the current process is a composition, or false if Vuo Editor.
 */
static bool VuoHeap_isComposition(void)
{
	static bool isComposition = false;
	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		isComposition = dlsym(RTLD_DEFAULT, "vuoCompositionStateKey");
	});
	return isComposition;
}
#endif

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
#ifdef VUOHEAP_TRACE
	vector<string> backtrace;
#endif
} VuoHeapEntry;

static map<const void *, VuoHeapEntry> *referenceCounts;  ///< The reference count for each pointer.
static set<const void *> *singletons;  ///< Known singleton pointers.

/**
 * Returns true if `pointer` looks like a valid pointer.
 *
 * @version200New
 */
static inline bool VuoHeap_isPointerValid(const void *pointer)
{
	// On macOS, memory allocated by `malloc` is 16-byte aligned,
	// so any non-16-byte-aligned pointers are suspicious.
	// https://opensource.apple.com/source/Libc/Libc-825.26/gen/malloc.3.auto.html says
	// "The allocated memory is aligned such that it can be used for any data type,
	// including AltiVec- and SSE-related types."
	// And https://software.intel.com/en-us/cpp-compiler-developer-guide-and-reference-alignment-support says
	// "When using the Intel® Streaming SIMD Extensions (Intel® SSE) intrinsics,
	// you should align data to 16 bytes in memory operations."
	if ((unsigned long)pointer & 0xf)
		return false;

	// On macOS, `malloc` shouldn't give us any pointers below 4 GB.
	// https://opensource.apple.com/source/ld64/ld64-242/doc/man/man1/ld.1.auto.html says
	// "By default the linker creates an unreadable segment starting at address zero named __PAGEZERO.
	// On 64-bit architectures, the default size is 4GB."
	if ((unsigned long)pointer < 0x100000000)
		return false;

	// x86_64 is currently limited to 48 bits (256 TB) of virtual address space.
	// http://support.amd.com/TechDocs/24593.pdf, page 131, says
	// "Bits 63:48 are a sign extension of bit 47".
	// This will no longer be the case if/when chips with 5-level paging are produced.
	if ((unsigned long)pointer > 0xffffffffffff)
		return false;

	return true;
}

/**
 * Returns true if `pointer` points to a memory page that's been allocated
 * (and therefore might be a valid pointer to 1 or more bytes of data).
 *
 * @version200New
 */
bool VuoHeap_isPointerReadable(const void *pointer)
{
	// Round down to the beginning of `pointer`'s heap page.
	// Assume getpagesize() returns a power-of-two; subtracting 1 turns it into a bitmask.
	int pageSize = getpagesize();
	long heapPageMask = ~((long)pageSize-1);
	long heapPage = (long)pointer & heapPageMask;

	// Try to load the page into core.
	mlock((void *)heapPage, pageSize);
	munlock((void *)heapPage, pageSize);

	// Check whether the page was successfully loaded into core.
	char pageStatus[1];
	if (mincore((void *)heapPage, pageSize, pageStatus) == 0)
		return pageStatus[0] & MINCORE_INCORE;

	else
		return false;
}

/**
 * If `pointer` refers to allocated memory, copies its first 16 printable characters into `summary`, followed by char 0.
 */
static void VuoHeap_makeSafePointerSummary(char *summary, const void *pointer)
{
	if (VuoHeap_isPointerReadable(pointer)
	 && VuoHeap_isPointerReadable((char *)pointer + 15))
	{
		// Page(s) are valid, so output the pointer's first 16 printable characters.
		char *pointerAsChar = (char *)pointer;
		for (int i = 0; i < 16; ++i)
			if (isprint(pointerAsChar[i]))
				summary[i] = pointerAsChar[i];
			else
				summary[i] = '_';
		summary[16] = 0;
	}
	else
		strlcpy(summary, "(not allocated)", 17);
}

/**
 * Returns a description of the specified @a heapPointer:
 * the file, line, and function where VuoRegister() was called,
 * and the variable name.
 *
 * The caller is responsible for freeing the returned string.
 */
static char *VuoHeap_makeDescription(VuoHeapEntry e)
{
#ifdef VUOHEAP_TRACE
	ostringstream oss;
	int i = 1;
	for (auto line : e.backtrace)
	{
		char *lineNumber;
		asprintf(&lineNumber, "%3d ", i++);
		oss << lineNumber << line << endl;
		free(lineNumber);
	}

	const char *format = "%s:%d :: %s() :: %s\n%s";
	int size = snprintf(NULL, 0, format,  e.file, e.line, e.function, e.variable, oss.str().c_str());
	char *description = (char *)malloc(size+1);
	snprintf(description, size+1, format, e.file, e.line, e.function, e.variable, oss.str().c_str());
#else
	const char *format = "%s:%d :: %s() :: %s";
	int size = snprintf(NULL, 0, format,  e.file, e.line, e.function, e.variable);
	char *description = (char *)malloc(size+1);
	snprintf(description, size+1, format, e.file, e.line, e.function, e.variable);
#endif
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

#if 0
	// Periodically dump the referenceCounts table, to help find leaks.
	const double dumpInterval = 5.0; // seconds
	dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
	dispatch_source_set_timer(timer, dispatch_walltime(NULL,0), NSEC_PER_SEC*dumpInterval, NSEC_PER_SEC*dumpInterval);
	dispatch_source_set_event_handler(timer, ^{
										  fprintf(stderr, "\n\n\n\n\nreferenceCounts:\n");
										  pthread_mutex_lock(&VuoHeap_mutex);
										  for (map<const void *, VuoHeapEntry>::iterator i = referenceCounts->begin(); i != referenceCounts->end(); ++i)
										  {
											  const void *heapPointer = i->first;
											  char pointerSummary[17];
											  VuoHeap_makeSafePointerSummary(pointerSummary, heapPointer);
											  char *description = VuoHeap_makeDescription(i->second);
											  fprintf(stderr, "\t% 3d refs to %p \"%s\", registered at %s\n", i->second.referenceCount, heapPointer, pointerSummary, description);
											  free(description);
										  }
										  pthread_mutex_unlock(&VuoHeap_mutex);
									  });
	dispatch_resume(timer);
#endif
}

/**
 * Sends a telemetry error with information about any objects remaining in the reference counting table.
 */
void VuoHeap_report(void)
{
	pthread_mutex_lock(&VuoHeap_mutex);

	if (! referenceCounts->empty())
	{
		ostringstream errorMessage;
		errorMessage << "On reference table " << referenceCounts
					 << ", VuoRelease was not called enough times for:" << endl;
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

	pthread_mutex_unlock(&VuoHeap_mutex);
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
 * @param linenumber The line in the file in which VuoRegister() is called.
 * @param func The function in which VuoRegister() is called.
 * @param pointerName The stringified pointer variable name.
 * @return The updated reference count of @a heapPointer. This is 0 if @a heapPointer is not already being reference-counted, greater than 0 if it is, or -1 if @a heapPointer is null.
 */
int VuoRegisterF(const void *heapPointer, DeallocateFunctionType deallocate, const char *file, unsigned int linenumber, const char *func, const char *pointerName)
{
	if (! heapPointer)
		return -1;

	if (!VuoHeap_isPointerValid(heapPointer))
	{
		ostringstream errorMessage;
		char *description = VuoHeap_makeDescription((VuoHeapEntry){0, NULL, file, linenumber, func, pointerName,
#ifdef VUOHEAP_TRACE
			VuoLog_getBacktrace(),
#endif
		});
		errorMessage << "On reference table " << referenceCounts
					 << ", VuoRegister was called for bogus pointer " << heapPointer
					 << " " << description;
		free(description);
		sendErrorWrapper(errorMessage.str().c_str());
	}

	bool isAlreadyReferenceCounted;
	int updatedCount;

	pthread_mutex_lock(&VuoHeap_mutex);
	{
#ifdef VUOHEAP_TRACE
#ifdef VUOHEAP_TRACEALL
		if (VuoHeap_isComposition())
#else
		if (VuoHeap_trace->find(heapPointer) != VuoHeap_trace->end())
#endif
		{
			fprintf(stderr, "table=%p  VuoRegister(%p)  %s\n", referenceCounts, heapPointer, pointerName);
			VuoLog_backtrace();
		}
#endif

		isAlreadyReferenceCounted = referenceCounts->count(heapPointer);
		if (! isAlreadyReferenceCounted)
		{
			updatedCount = 0;
			(*referenceCounts)[heapPointer] = (VuoHeapEntry){updatedCount, deallocate, file, linenumber, func, pointerName,
#ifdef VUOHEAP_TRACE
				VuoLog_getBacktrace(),
#endif
			};
		}
		else
			updatedCount = (*referenceCounts)[heapPointer].referenceCount;
	}
	pthread_mutex_unlock(&VuoHeap_mutex);

	if (isAlreadyReferenceCounted)
	{
		ostringstream errorMessage;
		char *description = VuoHeap_makeDescription((VuoHeapEntry){0, NULL, file, linenumber, func, pointerName,
#ifdef VUOHEAP_TRACE
			VuoLog_getBacktrace(),
#endif
		});
		errorMessage << "On reference table " << referenceCounts
					 << ", VuoRegister was called more than once for " << heapPointer
					 << " " << description;
		free(description);
		sendErrorWrapper(errorMessage.str().c_str());
	}

	return updatedCount;
}

/**
 * Instead of this function, you probably want to use VuoRegisterSingleton(). This function is used to implement
 * the VuoRegisterSingleton() macro.
 */
int VuoRegisterSingletonF(const void *heapPointer, const char *file, unsigned int linenumber, const char *func, const char *pointerName)
{
	if (! heapPointer)
		return -1;

	if (!VuoHeap_isPointerValid(heapPointer))
	{
		ostringstream errorMessage;
		char *description = VuoHeap_makeDescription((VuoHeapEntry){0, NULL, file, linenumber, func, pointerName,
#ifdef VUOHEAP_TRACE
			VuoLog_getBacktrace(),
#endif
		});
		errorMessage << "On reference table " << referenceCounts
					 << ", VuoRegisterSingleton was called for bogus pointer " << heapPointer
					 << " " << description;
		free(description);
		sendErrorWrapper(errorMessage.str().c_str());
	}

	bool isAlreadyReferenceCounted;

	pthread_mutex_lock(&VuoHeap_mutex);
	{
#ifdef VUOHEAP_TRACE
#ifdef VUOHEAP_TRACEALL
		if (VuoHeap_isComposition())
#else
		if (VuoHeap_trace->find(heapPointer) != VuoHeap_trace->end())
#endif
		{
			fprintf(stderr, "table=%p  VuoRegisterSingleton(%p)  %s\n", referenceCounts, heapPointer, pointerName);
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
	pthread_mutex_unlock(&VuoHeap_mutex);

	if (isAlreadyReferenceCounted)
	{
		ostringstream errorMessage;
		char *description = VuoHeap_makeDescription((VuoHeapEntry){0, NULL, file, linenumber, func, pointerName,
#ifdef VUOHEAP_TRACE
			VuoLog_getBacktrace(),
#endif
		});
		errorMessage << "On reference table " << referenceCounts
					 << ", VuoRegisterSingleton was called more than once for " << heapPointer
					 << " " << description;
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
extern "C" int VuoRetainF(const void *heapPointer, const char *file, unsigned int linenumber, const char *func)
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

	if (!VuoHeap_isPointerValid(heapPointer))
	{
		char pointerSummary[17];
		VuoHeap_makeSafePointerSummary(pointerSummary, heapPointer);
		ostringstream errorMessage;
		errorMessage << "On reference table " << referenceCounts
					 << ", VuoRetain was called for bogus pointer " << heapPointer
					 << " \"" << pointerSummary << "\"";
		sendErrorWrapper(errorMessage.str().c_str());
	}

	int updatedCount = -1;
	bool foundSingleton = false;

	pthread_mutex_lock(&VuoHeap_mutex);
	{
		map<const void *, VuoHeapEntry>::iterator i = referenceCounts->find(heapPointer);

#ifdef VUOHEAP_TRACE
#ifdef VUOHEAP_TRACEALL
		if (VuoHeap_isComposition())
#else
		if (VuoHeap_trace->find(heapPointer) != VuoHeap_trace->end())
#endif
		{
			fprintf(stderr, "table=%p  VuoRetain(%p)  %s\n", referenceCounts, heapPointer, (i != referenceCounts->end()) ? i->second.variable : "");
			VuoLog_backtrace();
		}
#endif

		if (i != referenceCounts->end())
			updatedCount = ++(i->second.referenceCount);
		else
			foundSingleton = singletons->find(heapPointer) != singletons->end();
	}
	pthread_mutex_unlock(&VuoHeap_mutex);

	if (updatedCount == -1 && !foundSingleton)
	{
		char pointerSummary[17];
		VuoHeap_makeSafePointerSummary(pointerSummary, heapPointer);
		ostringstream errorMessage;
		errorMessage << "On reference table " << referenceCounts
					 << ", VuoRetain was called for unregistered pointer " << heapPointer
					 << " \"" << pointerSummary << "\"";
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
extern "C" int VuoReleaseF(const void *heapPointer, const char *file, unsigned int linenumber, const char *func)
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

	if (!VuoHeap_isPointerValid(heapPointer))
	{
		char pointerSummary[17];
		VuoHeap_makeSafePointerSummary(pointerSummary, heapPointer);
		ostringstream errorMessage;
		errorMessage << "On reference table " << referenceCounts
					 << ", VuoRelease was called for bogus pointer " << heapPointer
					 << " \"" << pointerSummary << "\"";
		sendErrorWrapper(errorMessage.str().c_str());
	}

	int updatedCount = -1;
	bool foundSingleton = false;
	bool isRegisteredWithoutRetain = false;
	DeallocateFunctionType deallocate = NULL;

	pthread_mutex_lock(&VuoHeap_mutex);
	{
		map<const void *, VuoHeapEntry>::iterator i = referenceCounts->find(heapPointer);

#ifdef VUOHEAP_TRACE
#ifdef VUOHEAP_TRACEALL
		if (VuoHeap_isComposition())
#else
		if (VuoHeap_trace->find(heapPointer) != VuoHeap_trace->end())
#endif
		{
			fprintf(stderr, "table=%p  VuoRelease(%p)  %s\n", referenceCounts, heapPointer, (i != referenceCounts->end()) ? i->second.variable : "");
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

#ifdef VUOHEAP_TRACE
		if (updatedCount == 0)
		{
#ifdef VUOHEAP_TRACEALL
			if (VuoHeap_isComposition())
			{
#else
			if (VuoHeap_trace->find(heapPointer) != VuoHeap_trace->end())
			{
				VuoHeap_trace->erase(heapPointer);
#endif
				fprintf(stderr, "table=%p  VuoDeallocate(%p)\n", referenceCounts, heapPointer);
//				VuoLog_backtrace();
			}
		}
#endif
	}
	pthread_mutex_unlock(&VuoHeap_mutex);

	if (updatedCount == 0)
		deallocate((void *)heapPointer);
	else if (updatedCount == -1 && !foundSingleton)
	{
		char pointerSummary[17];
		VuoHeap_makeSafePointerSummary(pointerSummary, heapPointer);
		ostringstream errorMessage;
		errorMessage << "On reference table " << referenceCounts
					 << ", VuoRelease was called for "
					 << (isRegisteredWithoutRetain ? "unretained" : "unregistered")
					 << " pointer " << heapPointer
					 << " \"" << pointerSummary << "\"";
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
char *VuoHeap_getDescription(const void *heapPointer)
{
	char *description = nullptr;

	pthread_mutex_lock(&VuoHeap_mutex);

	map<const void *, VuoHeapEntry>::iterator i = referenceCounts->find(heapPointer);
	if (i != referenceCounts->end())
		description = VuoHeap_makeDescription(i->second);

	pthread_mutex_unlock(&VuoHeap_mutex);

	if (description)
		return description;
	else
		return strdup("(pointer was not VuoRegister()ed)");
}

/**
 * Pass a pointer to this function to log all its subsequent retains and releases.
 *
 * This only has an effect when CMake option `VUO_HEAP_TRACE` is enabled.
 */
void VuoHeap_addTrace(const void *heapPointer)
{
	pthread_mutex_lock(&VuoHeap_mutex);

	VuoHeap_trace->insert(heapPointer);

	pthread_mutex_unlock(&VuoHeap_mutex);
}
