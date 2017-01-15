/**
 * @file
 * VuoDragEvent C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUODRAGEVENT_H
#define VUODRAGEVENT_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoDragEvent VuoDragEvent
 * Information about an in-progress or completed file drag.
 *
 * @{
 */

#include "VuoPoint2d.h"
#include "VuoUrl.h"
#include "VuoList_VuoUrl.h"
#include "VuoWindowReference.h"

/**
 * Information about an in-progress or completed file drag
 */
typedef struct
{
	VuoPoint2d position;
	VuoList_VuoUrl urls;

	char blah[42]; ///< @todo https://b33p.net/kosada/node/4124
} VuoDragEvent;

VuoDragEvent VuoDragEvent_makeFromJson(struct json_object *js);
struct json_object *VuoDragEvent_getJson(const VuoDragEvent value);
char *VuoDragEvent_getSummary(const VuoDragEvent value);

VuoDragEvent VuoDragEvent_make(const VuoPoint2d position, const VuoList_VuoUrl urls);

#define VuoDragEvent_SUPPORTS_COMPARISON
bool VuoDragEvent_areEqual(const VuoDragEvent value1, const VuoDragEvent value2);
bool VuoDragEvent_isLessThan(const VuoDragEvent value1, const VuoDragEvent value2);

void VuoWindowReference_addDragCallbacks(const VuoWindowReference wr,
										 void (*dragEnteredCallback)(VuoDragEvent e),
										 void (*dragMovedToCallback)(VuoDragEvent e),
										 void (*dragCompletedCallback)(VuoDragEvent e),
										 void (*dragExitedCallback)(VuoDragEvent e));
void VuoWindowReference_removeDragCallbacks(const VuoWindowReference wr,
											void (*dragEnteredCallback)(VuoDragEvent e),
											void (*dragMovedToCallback)(VuoDragEvent e),
											void (*dragCompletedCallback)(VuoDragEvent e),
											void (*dragExitedCallback)(VuoDragEvent e));

/**
 * Automatically generated function.
 */
///@{
VuoDragEvent VuoDragEvent_makeFromString(const char *str);
char *VuoDragEvent_getString(const VuoDragEvent value);
void VuoDragEvent_retain(VuoDragEvent value);
void VuoDragEvent_release(VuoDragEvent value);
///@}

/**
 * @}
 */

#endif
