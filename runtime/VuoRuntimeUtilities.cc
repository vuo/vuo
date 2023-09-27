/**
 * @file
 * VuoRuntimeUtilities implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRuntimeUtilities.hh"
#include "VuoRuntimeState.hh"

/**
 * Returns an integer hash for a C string.
 */
unsigned long VuoRuntimeUtilities::hash(const char *str)
{
	// sdbm algorithm (http://www.cse.yorku.ca/~oz/hash.html) —
	// very low probability of collisions (https://programmers.stackexchange.com/a/145633/38390)

	unsigned long hash = 0;
	int c;

	while ((c = *str++))
		hash = c + (hash << 6) + (hash << 16) - hash;

	return hash;
}

extern "C"
{

/**
 * Returns a numerical ID for the composition that is unique process-wide and persists across live-coding reloads.
 */
uint64_t vuoGetCompositionUniqueIdentifier(const struct VuoCompositionState *compositionState)
{
	if (compositionState)
		return (uint64_t)((VuoRuntimeState *)compositionState->runtimeState)->persistentState;
	else
		return 0;
}

/**
 * Returns a context for the trigger scheduler to pass to the trigger worker.
 */
void * vuoCreateTriggerWorkerContext(VuoCompositionState *compositionState, void *dataCopy, unsigned long *eventIdCopy)
{
	void **context = (void **)malloc(3 * sizeof(void *));
	context[0] = (void *)compositionState;
	context[1] = dataCopy;
	context[2] = (void *)eventIdCopy;
	return (void *)context;
}

/**
 * Frees the context created by @ref vuoCreateTriggerWorkerContext().
 */
void vuoFreeTriggerWorkerContext(void *context)
{
	void **contextArray = (void **)context;
	free(contextArray[1]);
	free(contextArray[2]);
	free(contextArray);
}

/**
 * Returns a context for `compositionSetPublishedInputPortValue()` to pass to its worker function.
 */
void * vuoCreatePublishedInputWorkerContext(VuoCompositionState *compositionState, const char *inputPortIdentifier, const char *valueAsString,
											bool isCompositionRunning)
{
	void **context = (void **)malloc(4 * sizeof(void *));
	context[0] = (void *)compositionState;
	context[1] = (void *)inputPortIdentifier;
	context[2] = (void *)valueAsString;
	context[3] = (void *)isCompositionRunning;
	return (void *)context;
}

/**
 * Returns the strings appended in order.
 */
char * vuoConcatenateStrings2(const char *s0, const char *s1)
{
	size_t bufferLength = strlen(s0) + strlen(s1) + 1;
	char *buffer = (char *)malloc(bufferLength);
	buffer[0] = 0;
	strlcat(buffer, s0, bufferLength);
	strlcat(buffer, s1, bufferLength);
	return buffer;
}

/**
 * Returns the strings appended in order.
 */
char * vuoConcatenateStrings3(const char *s0, const char *s1, const char *s2)
{
	size_t bufferLength = strlen(s0) + strlen(s1) + strlen(s2) + 1;
	char *buffer = (char *)malloc(bufferLength);
	buffer[0] = 0;
	strlcat(buffer, s0, bufferLength);
	strlcat(buffer, s1, bufferLength);
	strlcat(buffer, s2, bufferLength);
	return buffer;
}

/**
 * Returns the strings appended in order.
 */
char * vuoConcatenateStrings(const char **strings, size_t stringCount)
{
	size_t bufferLength = 1;
	for (size_t i = 0; i < stringCount; ++i)
		bufferLength += strlen(strings[i]);
	char *buffer = (char *)malloc(bufferLength);
	buffer[0] = 0;
	for (size_t i = 0; i < stringCount; ++i)
		strlcat(buffer, strings[i], bufferLength);
	return buffer;
}

}  // extern "C"
