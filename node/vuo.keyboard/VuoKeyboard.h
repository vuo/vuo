/**
 * @file
 * VuoKeyboard interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoKey.h"
#include "VuoModifierKey.h"

typedef void * VuoKeyboard;  ///< Handle returned when starting to listen for events, to be used when stopping listening.

VuoKeyboard * VuoKeyboard_make(void);

void VuoKeyboard_startListeningForTyping(VuoKeyboard *keyboardListener,
										 VuoOutputTrigger(typedLine, VuoText),
										 VuoOutputTrigger(typedWord, VuoText),
										 VuoOutputTrigger(typedCharacter, VuoText),
										 VuoWindowReference window);

void VuoKeyboard_startListeningForTypingWithCallback(	VuoKeyboard *keyboardListener,
														void (^typedLine)(VuoText),
														void (^typedWord)(VuoText),
														void (^typedCharacter)(VuoText, VuoModifierKey),
														VuoWindowReference window);

void VuoKeyboard_startListeningForButtons(VuoKeyboard *keyboardListener,
										  VuoOutputTrigger(pressed, void),
										  VuoOutputTrigger(released, void),
										  VuoWindowReference window,
										  VuoKey key,
										  VuoModifierKey modifierKey,
										  bool shouldFireForRepeat);

void VuoKeyboard_stopListening(VuoKeyboard *keyboardListener);
