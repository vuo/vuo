/**
 * @file
 * VuoInteractionType C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoInteractionType_h
#define VuoInteractionType_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoInteractionType VuoInteractionType
 * Describes the type of event that initiated this interaction.
 *
 * @{
 */

/**
 * Describes the type of event that initiated this interaction.
 *
 * @version200New
 */
typedef enum {
	VuoInteractionType_None,		// No change in this event.
	VuoInteractionType_Move,		// The position has moved.
	VuoInteractionType_Press,		// A press has been begun.
	VuoInteractionType_Release,		// A press has been released (note that this may not be called if the event is a click, or the event is canceled).
	VuoInteractionType_Click,		// A click has been detected (this will be called in place of a Release event).
	VuoInteractionType_DragStart,	// Drag has been initiated
	VuoInteractionType_Drag,		// The input is dragging.
	VuoInteractionType_DragFinish,	// Drag has been completed successfully.
	VuoInteractionType_Canceled		// Drag or mouse press has been canceled (usually means mouse went off-screen).
} VuoInteractionType;

#include "VuoList_VuoInteractionType.h"

VuoInteractionType VuoInteractionType_makeFromJson(struct json_object * js);
struct json_object * VuoInteractionType_getJson(const VuoInteractionType value);
VuoList_VuoInteractionType VuoInteractionType_getAllowedValues(void);
char * VuoInteractionType_getSummary(const VuoInteractionType value);

bool VuoInteractionType_areEqual(const VuoInteractionType a, const VuoInteractionType b);

/// @{
/**
 * Automatically generated function.
 */
char * VuoInteractionType_getString(const VuoInteractionType value);
void VuoInteractionType_retain(VuoInteractionType value);
void VuoInteractionType_release(VuoInteractionType value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif

#endif
