/**
 * @file
 * VuoCompositionState implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <dlfcn.h>
#include "VuoCompositionState.h"

/**
 * Creates a composition state. The composition state does not take ownership of @a runtimeState or @a compositionIdentifier.
 */
struct VuoCompositionState * vuoCreateCompositionState(void *runtimeState, const char *compositionIdentifier)
{
	struct VuoCompositionState *compositionState = (struct VuoCompositionState *)malloc(sizeof(struct VuoCompositionState));
	compositionState->runtimeState = runtimeState;
	compositionState->compositionIdentifier = compositionIdentifier;
	return compositionState;
}

/**
 * Gets the composition state's `runtimeState` field.
 */
void * vuoGetCompositionStateRuntimeState(struct VuoCompositionState *compositionState)
{
	return compositionState->runtimeState;
}

/**
 * Gets the composition state's `compositionIdentifier` field.
 */
const char * vuoGetCompositionStateCompositionIdentifier(struct VuoCompositionState *compositionState)
{
	return compositionState->compositionIdentifier;
}

/**
 * Frees the composition state (but not its fields).
 */
void vuoFreeCompositionState(struct VuoCompositionState *compositionState)
{
	free(compositionState);
}

/**
 * Key used to store and retrieve composition state from thread-local state.
 */
pthread_key_t vuoCompositionStateKey;

/**
 * Initializes `vuoCompositionStateKey`.
 */
static void __attribute__((constructor)) compositionStateKey_init(void)
{
	int ret = pthread_key_create(&vuoCompositionStateKey, NULL);
	if (ret)
		VUserLog("Couldn't create the key for storing the composition state in thread-local state: %s", strerror(errno));
}

/**
 * Stores @a compositionState in thread-local storage, making it available via a call to
 * @ref vuoCopyCompositionStateFromThreadLocalStorage() on the same thread.
 */
void vuoAddCompositionStateToThreadLocalStorage(const struct VuoCompositionState *compositionState)
{
	int ret = pthread_setspecific(vuoCompositionStateKey, compositionState);
	if (ret)
		VUserLog("Couldn't store the composition state in thread-local state: %s", strerror(errno));
}

/**
 * Removes the stored composition state (if any) in thread-local storage, making it no longer available
 * to @ref vuoCopyCompositionStateFromThreadLocalStorage().
 */
void vuoRemoveCompositionStateFromThreadLocalStorage(void)
{
	int ret = pthread_setspecific(vuoCompositionStateKey, NULL);
	if (ret)
		VUserLog("Couldn't store a null composition state in thread-local state: %s", strerror(errno));
}

/**
 * Retrieves a copy of a composition state previously stored by @ref vuoAddCompositionStateToThreadLocalStorage()
 * on the current thread, or null if no composition state is found.
 *
 * The returned `VuoCompositionState`'s `compositionIdentifier` field is null.
 */
void * vuoCopyCompositionStateFromThreadLocalStorage(void)
{
	void *compositionState = pthread_getspecific(vuoCompositionStateKey);
	if (! compositionState)
	{
		void **vuoRuntimeState = (void **) dlsym(RTLD_SELF, "vuoRuntimeState");
		if (!vuoRuntimeState)
			vuoRuntimeState = (void **) dlsym(RTLD_DEFAULT, "vuoRuntimeState");
		if (vuoRuntimeState && *vuoRuntimeState)
			VUserLog("Warning: Couldn't find the composition state in thread-local state.");
		return NULL;
	}

	struct VuoCompositionState *compositionStateCopy = (struct VuoCompositionState *)malloc(sizeof(struct VuoCompositionState));
	compositionStateCopy->runtimeState = ((struct VuoCompositionState *)compositionState)->runtimeState;
	compositionStateCopy->compositionIdentifier = NULL;
	return compositionStateCopy;
}
