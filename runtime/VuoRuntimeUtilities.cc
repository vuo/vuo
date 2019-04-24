/**
 * @file
 * VuoRuntimeUtilities implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRuntimeUtilities.hh"
#include "VuoRuntimeState.hh"

/**
 * Returns an integer hash for a C string.
 */
unsigned long VuoRuntimeUtilities::hash(const char *str)
{
	// sdbm algorithm (http://www.cse.yorku.ca/~oz/hash.html) —
	// very low probability of collisions (http://programmers.stackexchange.com/a/145633/38390)

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
 * Returns the strings appended in order.
 */
char * vuoConcatenateStrings2(const char *s0, const char *s1)
{
	size_t bufferLength = strlen(s0) + strlen(s1) + 1;
	char *buffer = (char *)malloc(bufferLength);
	buffer[0] = 0;
	strcat(buffer, s0);
	strcat(buffer, s1);
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
	strcat(buffer, s0);
	strcat(buffer, s1);
	strcat(buffer, s2);
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
		strcat(buffer, strings[i]);
	return buffer;
}

}  // extern "C"
