/**
 * @file
 * VuoHeap implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
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

/**
 * Calls the sendError() function defined in VuoRuntime (without introducing a direct dependency on VuoRuntime).
 */
void sendErrorWrapper(const char *message)
{
	typedef void (*sendErrorType)(const char *message);
	sendErrorType sendError = (sendErrorType) dlsym(RTLD_SELF, "sendError");  // for running composition in separate process as executable or in current process
	if (! sendError)
		sendError = (sendErrorType) dlsym(RTLD_DEFAULT, "sendError");  // for running composition in separate process as dynamic libraries
	sendError(message);
}

map<const void *, int> referenceCounts;  ///< The reference count for each pointer.
static set<const void *> singletons;  ///< Known singleton pointers.
map<const void *, DeallocateFunctionType> deallocateFunctions;  ///< The function to be used for deallocating each pointer.
map<const void *, string> descriptions;  ///< A human-readable description for each pointer.
dispatch_semaphore_t referenceCountsSemaphore = NULL;  ///< Synchronizes access to @ref referenceCounts.

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
 * Initializes the reference-counting system. To be called once, before any other reference-counting function calls.
 */
void VuoHeap_init(void)
{
	if (! referenceCountsSemaphore)
		referenceCountsSemaphore = dispatch_semaphore_create(1);

#if 0
	// Periodically dump the referenceCounts table, to help find leaks.
	const double dumpInterval = 5.0; // seconds
	dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
	dispatch_source_set_timer(timer, dispatch_walltime(NULL,0), NSEC_PER_SEC*dumpInterval, NSEC_PER_SEC*dumpInterval);
	dispatch_source_set_event_handler(timer, ^{
										  fprintf(stderr, "\n\n\n\n\nreferenceCounts:\n");
										  dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
										  for (map<const void *, int>::iterator i = referenceCounts.begin(); i != referenceCounts.end(); ++i)
										  {
											  const void *heapPointer = i->first;
											  int referenceCount = i->second;
											  string description = descriptions[heapPointer];
											  char pointerSummary[17];
											  VuoHeap_makeSafePointerSummary(pointerSummary, heapPointer);
											  fprintf(stderr, "\t% 3d refs to %p \"%s\", registered at %s\n", referenceCount, heapPointer, pointerSummary, description.c_str());
										  }
										  dispatch_semaphore_signal(referenceCountsSemaphore);
									  });
	dispatch_resume(timer);
#endif
}

/**
 * Cleans up the reference-counting system. To be called once, after all other reference-counting function calls.
 */
void VuoHeap_fini(void)
{
	if (! referenceCounts.empty())
	{
		ostringstream errorMessage;
		errorMessage << "VuoRelease was not called enough times for:" << endl;
		for (map<const void *, int>::iterator i = referenceCounts.begin(); i != referenceCounts.end(); ++i)
		{
			const void *heapPointer = i->first;
			int referenceCount = i->second;
			string description = descriptions[heapPointer];
			char pointerSummary[17];
			VuoHeap_makeSafePointerSummary(pointerSummary, heapPointer);
			errorMessage << "\t" << setw(3) << referenceCount << " refs to " << heapPointer << " \"" << pointerSummary << "\", registered at " << description << endl;
		}
		sendErrorWrapper(errorMessage.str().c_str());
	}

	dispatch_release(referenceCountsSemaphore);
	referenceCountsSemaphore = NULL;
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

	ostringstream sout;
	sout << file << ":" << line << " :: " << func << "() :: " << pointerName;
	string description = sout.str();
	string previousDescription;

	dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
	{

		isAlreadyReferenceCounted = (referenceCounts.find(heapPointer) != referenceCounts.end());
		if (! isAlreadyReferenceCounted)
		{
			deallocateFunctions[heapPointer] = deallocate;
			descriptions[heapPointer] = description;
		}
		else
		{
			previousDescription = descriptions[heapPointer];
		}
		updatedCount = referenceCounts[heapPointer];

	}
	dispatch_semaphore_signal(referenceCountsSemaphore);

	if (isAlreadyReferenceCounted)
	{
		ostringstream errorMessage;
		errorMessage << "VuoRegister was called more than once for " << heapPointer << " " <<
						previousDescription << " (previous call), " << description << " (current call)";
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

	ostringstream sout;
	sout << file << ":" << line << " :: " << func << "() :: " << pointerName;
	string description = sout.str();
	string previousDescription;

	dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
	{
		// Remove the singleton from the main reference-counting table, if it exists there.
		// Enables reclassifying a pointer that was already VuoRegister()ed.
		referenceCounts.erase(heapPointer);
		deallocateFunctions.erase(heapPointer);

		// Add the singleton to the singleton table.
		isAlreadyReferenceCounted = (singletons.find(heapPointer) != singletons.end());
		if (! isAlreadyReferenceCounted)
		{
			singletons.insert(heapPointer);
			descriptions[heapPointer] = description;
		}
		else
			previousDescription = descriptions[heapPointer];
	}
	dispatch_semaphore_signal(referenceCountsSemaphore);

	if (isAlreadyReferenceCounted)
	{
		ostringstream errorMessage;
		errorMessage << "VuoRegisterSingleton was called more than once for " << heapPointer << " " <<
						previousDescription << " (previous call), " << description << " (current call)";
		sendErrorWrapper(errorMessage.str().c_str());
	}

	return isAlreadyReferenceCounted ? 1 : 0;
}

/**
 * Instead of this function, you probably want to use VuoRetain(). This function is used to implement
 * the VuoRetain() macro.
 */
int VuoRetainF(const void *heapPointer, const char *file, unsigned int line, const char *func)
{
	if (! heapPointer)
		return -1;

	int updatedCount = -1;
	bool foundSingleton = false;
	string description;

	dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
	{

		map<const void *, int>::iterator i = referenceCounts.find(heapPointer);
		if (i != referenceCounts.end())
			updatedCount = ++referenceCounts[heapPointer];
		else
			foundSingleton = singletons.find(heapPointer) != singletons.end();
		description = descriptions[heapPointer];

	}
	dispatch_semaphore_signal(referenceCountsSemaphore);

	if (updatedCount == -1 && !foundSingleton)
	{
		ostringstream errorMessage;
		errorMessage << "VuoRetain was called by " << file << ":" << line << " :: " << func << "() for unregistered pointer " << heapPointer << " " << description;
		sendErrorWrapper(errorMessage.str().c_str());
	}

	return updatedCount;
}

/**
 * Instead of this function, you probably want to use VuoRelease(). This function is used to implement
 * the VuoRelease() macro.
 */
int VuoReleaseF(const void *heapPointer, const char *file, unsigned int line, const char *func)
{
	if (! heapPointer)
		return -1;

	int updatedCount = -1;
	bool foundSingleton = false;
	bool isRegisteredWithoutRetain = false;
	DeallocateFunctionType deallocate = NULL;
	string description;

	dispatch_semaphore_wait(referenceCountsSemaphore, DISPATCH_TIME_FOREVER);
	{

		map<const void *, int>::iterator i = referenceCounts.find(heapPointer);
		if (i != referenceCounts.end())
		{
			description = descriptions[heapPointer];
			if (referenceCounts[heapPointer] == 0)
			{
				isRegisteredWithoutRetain = true;
			}
			else
			{
				updatedCount = --referenceCounts[heapPointer];

				if (updatedCount == 0)
				{
					referenceCounts.erase(heapPointer);
					deallocate = deallocateFunctions[heapPointer];
					deallocateFunctions.erase(heapPointer);
					descriptions.erase(heapPointer);
				}
			}
		}
		else
			foundSingleton = singletons.find(heapPointer) != singletons.end();

	}
	dispatch_semaphore_signal(referenceCountsSemaphore);

	if (updatedCount == 0)
	{
		deallocate((void *)heapPointer);
	}
	else if (updatedCount == -1 && !foundSingleton)
	{
		ostringstream errorMessage;
		errorMessage << "VuoRelease was called by " << file << ":" << line << " :: " << func << "() for "
					 << (isRegisteredWithoutRetain ? "unretained" : "unregistered")
					 << " pointer " << heapPointer << " " << description;
		sendErrorWrapper(errorMessage.str().c_str());
	}

	return updatedCount;
}

/**
 * Returns a description of the specified @a heapPointer:
 * the file, line, and function where VuoRegister() was called,
 * and the variable name.
 *
 * The returned string is still owned by VuoHeap; you should not free it.
 */
const char * VuoHeap_getDescription(const void *heapPointer)
{
	map<const void *, string>::iterator i = descriptions.find(heapPointer);
	if (i != descriptions.end())
		return descriptions[heapPointer].c_str();
	else
		return "(pointer was not VuoRegister()ed)";
}
