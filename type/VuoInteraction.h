/**
 * @file
 * VuoInteraction C type definition.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoReal.h"
#include "VuoText.h"
#include "VuoPoint2d.h"
#include "VuoUuid.h"
#include "VuoInteractionType.h"
#include "VuoBoolean.h"

/// @{ List type.
typedef const struct VuoList_VuoInteraction_struct { void *l; } * VuoList_VuoInteraction;
#define VuoList_VuoInteraction_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoInteraction VuoInteraction
 * Holds information about an input device's status.
 *
 * @{
 */

/**
 * Holds information about an input device's status.
 *
 * @version200New
 */
typedef struct
{
	VuoUuid uuid;				// Globally unique identifier.
	VuoPoint2d position; 		// The x,y coordinates of this input relative to a window.
	bool isPressed; 			// Set true when this input device is in a "down" state - ex, mouse is depressed, or leap motion finger is past the z input wall.
	VuoInteractionType type;	// What kind of interaction is this.
	VuoPoint2d origin;			// The position at the time of the last onPressed event.
	double timestamp;			// The timestamp that the last onPressed event was fired.
	VuoInteger clickCount;		// The number of successive clicks with double-click timing.
} VuoInteraction;

VuoInteraction VuoInteraction_makeFromJson(struct json_object * js);
struct json_object * VuoInteraction_getJson(const VuoInteraction value);
char * VuoInteraction_getSummary(const VuoInteraction value);

bool VuoInteraction_update(const VuoPoint2d position, const VuoBoolean isPressed, VuoInteraction *interaction) VuoWarnUnusedResult;

/**
 * Returns a new VuoInteraction.
 */
static inline VuoInteraction VuoInteraction_make(void) __attribute__((const));
static inline VuoInteraction VuoInteraction_make(void)
{
	VuoInteraction i;

	i.uuid = VuoUuid_make();
	i.position = VuoPoint2d_make(0,0);
	i.isPressed = false;
	i.type = VuoInteractionType_None;
	i.origin = VuoPoint2d_make(0,0);
	i.timestamp = -1;
	i.clickCount = 0;

	return i;
}

#define VuoInteraction_SUPPORTS_COMPARISON

bool VuoInteraction_areEqual(const VuoInteraction a, const VuoInteraction b);
bool VuoInteraction_isLessThan(const VuoInteraction a, const VuoInteraction b);

/// @{
/**
 * Automatically generated function.
 */
char * VuoInteraction_getString(const VuoInteraction value);
void VuoInteraction_retain(VuoInteraction value);
void VuoInteraction_release(VuoInteraction value);
/// @}

/**
 * @}
 */
