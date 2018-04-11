/**
 * @file
 * VuoCompositionState interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>
#include <pthread.h>

/**
 * Runtime information about a composition or subcomposition.
 *
 * This data structure encapsulates information that is shared throughout the entire composition (`vuoRuntimeState`)
 * and information that is specific to one composition or subcomposition (`compositionIdentifier`). The reason for
 * encapsulating these data items in one structure is to enable a composition to easily pass the information
 * (including any data items that might be added in the future) down to subcompositions via function arguments.
 */
struct VuoCompositionState
{
	void *runtimeState;  ///< The VuoRuntimeState of the top-level composition.
	const char *compositionIdentifier;  ///< The identifier of this (sub)composition, unique among the top-level composition and its subcompositions.
};

struct VuoCompositionState * vuoCreateCompositionState(void *runtimeState, const char *compositionIdentifier);
void * vuoGetCompositionStateRuntimeState(struct VuoCompositionState *compositionState);
const char * vuoGetCompositionStateCompositionIdentifier(struct VuoCompositionState *compositionState);
void vuoFreeCompositionState(struct VuoCompositionState *compositionState);

extern pthread_key_t vuoCompositionStateKey;
void vuoAddCompositionStateToThreadLocalStorage(const struct VuoCompositionState *compositionState);
void vuoRemoveCompositionStateFromThreadLocalStorage(void);
const void * vuoGetCompositionStateFromThreadLocalStorage(void);

uint64_t vuoGetCompositionUniqueIdentifier(const struct VuoCompositionState *compositionState);

#ifdef __cplusplus
}
#endif
