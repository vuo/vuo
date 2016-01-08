/**
 * @file
 * VuoWindowTextInternal implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoWindowTextInternal.h"
#import "VuoWindowApplication.h"

#include <dispatch/dispatch.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindowTextInternal",
					 "dependencies" : [
						"AppKit.framework"
					 ]
				 });
#endif


@implementation VuoWindowTextInternal

@synthesize textView;
@synthesize scrollView;
@synthesize textFont;

/**
 * Creates a window containing a text view.
 *
 * @threadMain
 */
- (id)init
{
	NSRect frame = NSMakeRect(0, 0, 400, 600);
	NSUInteger styleMask = NSTitledWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
	NSRect contentRect = [NSWindow contentRectForFrameRect:frame styleMask:styleMask];

	if (self = [super initWithContentRect:contentRect
								styleMask:styleMask
								  backing:NSBackingStoreBuffered
									defer:NO])
	{
		triggersEnabled = NO;
		typedLine = NULL;
		typedWord = NULL;
		typedCharacter = NULL;

		[self setTitle:@"Vuo Console"];

		NSFont *_textFont = [NSFont fontWithName:@"Monaco" size:0];
		self.textFont = _textFont;

		NSScrollView *_scrollView = [[NSScrollView alloc] initWithFrame:[[self contentView] frame]];
		self.scrollView = _scrollView;
		[_scrollView release];
		[scrollView setHasVerticalScroller:YES];
		[scrollView setAutohidesScrollers:NO];
		[scrollView setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
		[self setContentView:scrollView];

		NSTextView *_textView = [[NSTextView alloc] initWithFrame:[[scrollView contentView] frame]];
		self.textView = _textView;
		[_textView release];
		[textView setFont:textFont];
		[textView setEditable:YES];
		[textView setAllowsUndo:NO];
		[textView setAutoresizingMask:NSViewWidthSizable];
		[scrollView setDocumentView:textView];

		textView.delegate = self;
	}
	return self;
}

/**
 * Handles the user's typing in the text view by:
 *  - only allowing the user to append text to the end, not modify the text above.
 *  - firing events for each character, word, and line appended.
 *
 * This method is only called when text is typed, not when text is modified programmatically.
 */
- (BOOL)textView:(NSTextView *)aTextView shouldChangeTextInRange:(NSRange)affectedCharRange replacementString:(NSString *)replacementString
{
	// Allow inserting multi-keystroke (e.g. accented) characters.
	NSUInteger markedTextLength = [aTextView markedRange].length;

	// Disallow inserting characters anywhere but at the end.
	if (affectedCharRange.location + markedTextLength != [[textView string] length])
		return NO;

	if (! triggersEnabled)
		return YES;  /// @todo Instead of skipping events, wait to fire them until triggers are enabled. (https://b33p.net/kosada/node/6196)

	// Don't fire events for partial multi-keystroke characters.
	if ([aTextView hasMarkedText] && markedTextLength == 0)
		return YES;

	for (NSUInteger i = 0; i < [replacementString length]; ++i)
	{
		unichar charTyped = [replacementString characterAtIndex:i];

		// Fire event for character.
		VuoText charTypedAsText = VuoText_make([[NSString stringWithCharacters:&charTyped length:1] UTF8String]);
		typedCharacter(charTypedAsText);

		// If user typed whitespace...
		if ([[NSCharacterSet whitespaceAndNewlineCharacterSet] characterIsMember:charTyped])
		{
			// ... and if it ends a word, then fire event for word.
			NSRange previousWhitespaceOrNewline = [[textView string] rangeOfCharacterFromSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]
																							  options:NSBackwardsSearch];
			NSUInteger previousWordStart = (previousWhitespaceOrNewline.location != NSNotFound ?
																						   previousWhitespaceOrNewline.location + 1 :
																						   0);
			if (previousWordStart < [[textView string] length])
			{
				NSString *wordTyped = [[textView string] substringFromIndex:previousWordStart];
				VuoText wordTypedAsText = VuoText_make([wordTyped UTF8String]);
				typedWord(wordTypedAsText);
			}
		}

		// If user typed a newline...
		if ([[NSCharacterSet newlineCharacterSet] characterIsMember:charTyped])
		{
			// ... and if it ends a line, then fire event for line.
			NSRange previousNewline = [[textView string] rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]
																				  options:NSBackwardsSearch];
			NSUInteger previousLineStart = (previousNewline.location != NSNotFound ?
																			   previousNewline.location + 1 :
																			   0);
			if (previousLineStart < [[textView string] length])
			{
				NSString *lineTyped = [[textView string] substringFromIndex:previousLineStart];
				VuoText lineTypedAsText = VuoText_make([lineTyped UTF8String]);
				typedLine(lineTypedAsText);
			}
		}
	}

	return YES;
}

/**
 * Updates the menu bar with this window's (lack of) menus.
 */
- (void)becomeMainWindow
{
	[super becomeMainWindow];

	[(VuoWindowApplication *)NSApp replaceWindowMenu:nil];
}

/**
 * Sets up the window to call trigger functions.
 */
- (void)enableTriggersWithTypedLine:(void (*)(VuoText))_typedLine
						  typedWord:(void (*)(VuoText))_typedWord
					 typedCharacter:(void (*)(VuoText))_typedCharacter
{
	triggersEnabled = YES;
	typedLine = _typedLine;
	typedWord = _typedWord;
	typedCharacter = _typedCharacter;
}

/**
 * Stops the window from calling trigger functions.
 */
- (void)disableTriggers
{
	triggersEnabled = NO;
	typedLine = NULL;
	typedWord = NULL;
	typedCharacter = NULL;
}

/**
 * Appends text and a linebreak to the text view. If needed, scrolls down to reveal the last line.
 */
- (void)appendLine:(const char *)text
{
	NSString *line = [[NSString stringWithUTF8String:text] stringByAppendingString:@"\n"];
	NSDictionary *attributes = [NSDictionary dictionaryWithObject:textFont forKey:NSFontAttributeName];
	NSAttributedString *attributedLine = [[NSAttributedString alloc] initWithString:line attributes:attributes];
	[[textView textStorage] appendAttributedString:attributedLine];
	[attributedLine release];

	NSTextContainer *textContainer = [textView textContainer];
	NSLayoutManager *layoutManager = [textView layoutManager];
	[layoutManager ensureLayoutForTextContainer:textContainer];
	NSRect frameToFitText = [layoutManager usedRectForTextContainer:textContainer];
	[textView setFrame:frameToFitText];

	[textView scrollRangeToVisible:NSMakeRange([[textView string] length], 0)];
}

@end
