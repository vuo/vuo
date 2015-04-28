/**
 * @file
 * VuoMouse interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoModifierKey.h"
#include "VuoMouseButton.h"

typedef void * VuoMouse;  ///< Handle returned when starting to listen for events, to be used when stopping listening.

VuoMouse * VuoMouse_make(void);

void VuoMouse_startListeningForScrolls(VuoMouse *mouseListener, void (*scrolled)(VuoPoint2d), VuoModifierKey modifierKey);
void VuoMouse_startListeningForMoves(VuoMouse *mouseListener, void (*movedTo)(VuoPoint2d),
									 VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForDeltas(VuoMouse *mouseListener, void (*movedBy)(VuoPoint2d),
									  VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForDrags(VuoMouse *mouseListener, void (*dragMovedTo)(VuoPoint2d),
									 VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForDragsWithCallback(VuoMouse *mouseListener, void (^dragMovedTo)(VuoPoint2d),
												 VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForPresses(VuoMouse *mouseListener, void (*pressed)(VuoPoint2d),
									   VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForPressesWithCallback(VuoMouse *mouseListener, void (^pressed)(VuoPoint2d),
												   VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForReleases(VuoMouse *mouseListener, void (*released)(VuoPoint2d),
										VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForReleasesWithCallback(VuoMouse *mouseListener, void (^released)(VuoPoint2d),
													VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey);
void VuoMouse_startListeningForClicks(VuoMouse *mouseListener, void (*singleClicked)(VuoPoint2d), void (*doubleClicked)(VuoPoint2d),
									  void (*tripleClicked)(VuoPoint2d),
									  VuoMouseButton button, VuoWindowReference window, VuoModifierKey modifierKey);

void VuoMouse_stopListening(VuoMouse *mouseListener);

VuoPoint2d VuoMouse_getPosition(VuoWindowReference window);
VuoBoolean VuoMouse_isPressed(VuoMouseButton button, VuoModifierKey modifierKey);
