/**
 * @file
 * VuoWindowTextInternal interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#import <AppKit/AppKit.h>

/**
 * A console window for use by Vuo node classes.
 */
@interface VuoWindowTextInternal : NSWindow <NSTextViewDelegate>
{
	BOOL triggersEnabled;  ///< True if the window can call its trigger callbacks.
	void (* typedLine)(VuoText);  ///< Trigger that fires when a line has been typed.
	void (* typedWord)(VuoText);  ///< Trigger that fires when a word has been typed.
	void (* typedCharacter)(VuoText);  ///< Trigger that fires when a character has been typed.
}

@property(retain) NSTextView *textView;  ///< The text view inside this window.
@property(retain) NSScrollView *scrollView;  ///< The scroll view that holds the text view.
@property(retain) NSFont *textFont;  ///< The font used inside the text view.

- (void)enableTriggersWithTypedLine:(void (*)(VuoText))typedLine
						  typedWord:(void (*)(VuoText))typedWord
					 typedCharacter:(void (*)(VuoText))typedCharacter;
- (void)disableTriggers;
- (void)appendLine:(const char *)text;

@end
