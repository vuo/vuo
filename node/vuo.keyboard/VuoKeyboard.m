/**
 * @file
 * VuoKeyboard implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#define NS_RETURNS_INNER_POINTER
#include <AppKit/AppKit.h>

#include "VuoKeyboard.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoKeyboard",
					"dependencies" : [
						"VuoKey",
						"VuoModifierKey",
						"VuoText",
						"VuoWindowReference",
						"AppKit.framework"
					]
				 });
#endif


/**
 * Handle for starting and stopping event listeners.
 */
struct VuoKeyboardContext
{
	id monitor;  ///< The handle returned by NSEvent's method to start monitoring events, to be used to stop monitoring.

	NSMutableString *wordInProgress;  ///< For typing listeners, the latest partial word typed.
	NSMutableString *lineInProgress;  ///< For typing listeners, the latest partial line typed.
	UInt32 deadKeyState;  ///< For typing listeners, the latest dead keys typed, possibly as part of a multi-keystroke character.
};

/**
 * Creates a handle for starting and stopping event listeners.
 */
VuoKeyboard * VuoKeyboard_make(void)
{
	struct VuoKeyboardContext *context = (struct VuoKeyboardContext *)calloc(1, sizeof(struct VuoKeyboardContext));
	VuoRegister(context, free);
	return (VuoKeyboard *)context;
}


/**
 * If the keypress event is not ignored, calls the trigger function(s) if this event completes a
 * character (Unicode), word, and/or line.
 */
static void VuoKeyboard_fireTypingIfNeeded(NSEvent *event,
										   struct VuoKeyboardContext *context,
										   VuoOutputTrigger(typedLine, VuoText),
										   VuoOutputTrigger(typedWord, VuoText),
										   VuoOutputTrigger(typedCharacter, VuoText),
										   VuoWindowReference windowRef)
{
	NSWindow *targetWindow = (NSWindow *)windowRef;
	if (! targetWindow || targetWindow == [event window] || [[targetWindow contentView] isInFullScreenMode])
	{
		char *unicodeBytes = VuoKey_getCharactersForMacVirtualKeyCode([event keyCode],
																	  [event modifierFlags],
																	  &context->deadKeyState);
		if (!unicodeBytes)
			return;
		NSString *unicodeString = [NSString stringWithUTF8String:unicodeBytes];

		for (NSUInteger i = 0; i < [unicodeString length]; ++i)
		{
			// Typed a character (e.g. Option-E-E for "é" or Option-E-Space for "ˆ", not just Option-E or Option).
			NSString *characterAsString = [unicodeString substringWithRange:NSMakeRange(i, 1)];
			VuoText character = VuoText_make([characterAsString UTF8String]);
			typedCharacter(character);

			unichar characterAsUnichar = [characterAsString characterAtIndex:0];
			if ([[NSCharacterSet whitespaceAndNewlineCharacterSet] characterIsMember:characterAsUnichar])
			{
				// Typed a whitespace character...
				if ([context->wordInProgress length] > 0)
				{
					// ... that completes a word.
					VuoText word = VuoText_make([context->wordInProgress UTF8String]);
					typedWord(word);
				}

				[context->wordInProgress deleteCharactersInRange:NSMakeRange(0, [context->wordInProgress length])];
			}
			else
			{
				[context->wordInProgress appendString:characterAsString];
			}

			if ([[NSCharacterSet newlineCharacterSet] characterIsMember:characterAsUnichar])
			{
				// Typed a newline character that completes a line.
				VuoText line = VuoText_make([context->lineInProgress UTF8String]);
				typedLine(line);

				[context->lineInProgress deleteCharactersInRange:NSMakeRange(0, [context->lineInProgress length])];
			}
			else
			{
				[context->lineInProgress appendString:characterAsString];
			}
		}
	}
}

/**
 * If the key event is not ignored, calls one of the trigger functions.
 */
static void VuoKeyboard_fireButtonsIfNeeded(NSEvent *event,
											VuoOutputTrigger(pressed, void),
											VuoOutputTrigger(released, void),
											VuoWindowReference windowRef,
											VuoKey key,
											VuoModifierKey modifierKey,
											bool shouldFireForRepeat)
{
	NSWindow *targetWindow = (NSWindow *)windowRef;
	NSEventType type = [event type];

	bool isARepeat = false;
	if (type == NSKeyDown || type == NSKeyUp)
		isARepeat = [event isARepeat];

	if ((! targetWindow || targetWindow == [event window] || [[targetWindow contentView] isInFullScreenMode]) &&
			(VuoKey_doesMacVirtualKeyCodeMatch([event keyCode], key)) &&
			(shouldFireForRepeat || !isARepeat))
	{
		CGEventFlags flags = CGEventGetFlags([event CGEvent]);
		bool isKeyInFlags = (type == NSFlagsChanged &&
							 (((key == VuoKey_Command) && (flags & kCGEventFlagMaskCommand)) ||
							  ((key == VuoKey_CapsLock) && (flags & kCGEventFlagMaskAlphaShift)) ||
							  ((key == VuoKey_Shift || key == VuoKey_RightShift) && (flags & kCGEventFlagMaskShift)) ||
							  ((key == VuoKey_Control || key == VuoKey_RightControl) && (flags & kCGEventFlagMaskControl)) ||
							  ((key == VuoKey_Option || key == VuoKey_RightOption) && (flags & kCGEventFlagMaskAlternate)) ||
							  ((key == VuoKey_Function) && (flags & kCGEventFlagMaskSecondaryFn))));

		if (VuoModifierKey_doMacEventFlagsMatch(CGEventGetFlags([event CGEvent]), modifierKey) || isKeyInFlags)
		{
			if (type == NSKeyDown || isKeyInFlags)
				pressed();
			else
				released();
		}
	}
}


/**
 * Starts listening for key presses, and calling the trigger function each time a character (Unicode), word, or line is typed.
 */
void VuoKeyboard_startListeningForTyping(VuoKeyboard *keyboardListener,
										 VuoOutputTrigger(typedLine, VuoText),
										 VuoOutputTrigger(typedWord, VuoText),
										 VuoOutputTrigger(typedCharacter, VuoText),
										 VuoWindowReference window)
{
	struct VuoKeyboardContext *context = (struct VuoKeyboardContext *)keyboardListener;
	context->wordInProgress = [NSMutableString new];
	context->lineInProgress = [NSMutableString new];

	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSKeyDownMask handler:^(NSEvent *event) {
		VuoKeyboard_fireTypingIfNeeded(event, context, typedLine, typedWord, typedCharacter, window);
		return event;
	}];

	context->monitor = monitor;
}

/**
 * Starts listening or key presses and releases, and calling the trigger function for each one.
 */
void VuoKeyboard_startListeningForButtons(VuoKeyboard *keyboardListener,
										  VuoOutputTrigger(pressed, void),
										  VuoOutputTrigger(released, void),
										  VuoWindowReference window,
										  VuoKey key,
										  VuoModifierKey modifierKey,
										  bool shouldFireForRepeat)
{
	struct VuoKeyboardContext *context = (struct VuoKeyboardContext *)keyboardListener;

	id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSKeyDownMask|NSKeyUpMask|NSFlagsChangedMask handler:^(NSEvent *event) {
		VuoKeyboard_fireButtonsIfNeeded(event, pressed, released, window, key, modifierKey, shouldFireForRepeat);
		return event;
	}];

	context->monitor = monitor;
}


/**
 * Stops listening for keyboard events for the given handle.
 */
void VuoKeyboard_stopListening(VuoKeyboard *keyboardListener)
{
	struct VuoKeyboardContext *context = (struct VuoKeyboardContext *)keyboardListener;

	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{  // wait for any in-progress monitor handlers to complete
						VUOLOG_PROFILE_END(mainQueue);
						if (context->monitor)
						{
							[NSEvent removeMonitor:context->monitor];
							context->monitor = nil;
						}
				  });

	[context->wordInProgress release];
	[context->lineInProgress release];
}
