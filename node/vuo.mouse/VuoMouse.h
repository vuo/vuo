/**
 * @file
 * VuoMouse interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoBoolean.h"
#include "VuoModifierKey.h"
#include "VuoMouseButton.h"
#include "VuoPoint2d.h"
#include "VuoWindowReference.h"
#include "VuoList_VuoPoint2d.h"

typedef void * VuoMouse;  ///< Handle returned when starting to listen for events, to be used when stopping listening.

VuoMouse * VuoMouse_make(void);

void VuoMouse_GetScreenDimensions(int64_t *width, int64_t *height);

void VuoMouse_startListeningForScrolls(VuoMouse *mouseListener, void (*scrolled)(VuoPoint2d),
									   VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForScrollsWithCallback(VuoMouse *mouseListener, void (^scrolled)(VuoPoint2d),
												   VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForMoves(VuoMouse *mouseListener, void (*movedTo)(VuoPoint2d),
									 VuoWindowReference window, VuoModifierKey modifierKey, bool global);
void VuoMouse_startListeningForMovesWithCallback(VuoMouse *mouseListener, void (^movedTo)(VuoPoint2d),
									 VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForDeltas(VuoMouse *mouseListener, void (*movedBy)(VuoPoint2d),
									  VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForDrags(VuoMouse *mouseListener, void (*dragMovedTo)(VuoPoint2d),
									 VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForDragsWithCallback(VuoMouse *mouseListener, void (^dragMovedTo)(VuoPoint2d),
												 VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey, bool fireRegardlessOfPosition);
void VuoMouse_startListeningForPresses(VuoMouse *mouseListener, void (*pressed)(VuoPoint2d), void (*forcePressed)(VuoPoint2d),
									   VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForPressesWithCallback(VuoMouse *mouseListener, void (^pressed)(VuoPoint2d), void (^forcePressed)(VuoPoint2d),
												   VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForPressureChanges(VuoMouse *mouseListener, void (*pressureChanged)(VuoReal), VuoMouseButton button, VuoModifierKey modifierKey);
void VuoMouse_startListeningForReleases(VuoMouse *mouseListener, void (*released)(VuoPoint2d),
										VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey, bool fireRegardlessOfPosition);
void VuoMouse_startListeningForReleasesWithCallback(VuoMouse *mouseListener, void (^released)(VuoPoint2d),
													VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey, bool fireRegardlessOfPosition);
void VuoMouse_startListeningForClicks(VuoMouse *mouseListener, void (*singleClicked)(VuoPoint2d), void (*doubleClicked)(VuoPoint2d),
									  void (*tripleClicked)(VuoPoint2d),
									  VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForTouches(VuoMouse *mouseListener,
	void (*touchesMoved)(VuoList_VuoPoint2d),
	void (*zoomed)(VuoReal),
	void (*swipedLeft)(void),
	void (*swipedRight)(void),
	VuoWindowReference window);

void VuoMouse_stopListening(VuoMouse *mouseListener);

void VuoMouseStatus_use(void);
void VuoMouseStatus_disuse(void);
bool VuoMouse_getStatus(VuoPoint2d *position, VuoBoolean *isPressed,
						VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey,
						bool onlyUpdateWhenActive) VuoWarnUnusedResult;

#ifdef _APPKITDEFINES_H
VuoPoint2d VuoMouse_convertWindowToVuoCoordinates(NSPoint pointInWindow, NSWindow *window, bool *shouldFire);
#endif

#ifdef __cplusplus
}
#endif
