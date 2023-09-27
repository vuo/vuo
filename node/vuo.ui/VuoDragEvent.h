/**
 * @file
 * VuoDragEvent C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

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

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Information about an in-progress or completed file drag
 */
typedef struct
{
	VuoPoint2d position;
	VuoList_VuoUrl urls;
} VuoDragEvent;

#define VuoDragEvent_SUPPORTS_COMPARISON

VuoDragEvent VuoDragEvent_makeFromJson(struct json_object *js);
struct json_object *VuoDragEvent_getJson(const VuoDragEvent value);
char *VuoDragEvent_getSummary(const VuoDragEvent value);

VuoDragEvent VuoDragEvent_make(const VuoPoint2d position, const VuoList_VuoUrl urls);

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

void VuoDragEvent_retain(VuoDragEvent value);
void VuoDragEvent_release(VuoDragEvent value);

/**
 * Automatically generated function.
 */
///@{
char *VuoDragEvent_getString(const VuoDragEvent value);
///@}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
