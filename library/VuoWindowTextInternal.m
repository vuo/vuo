/**
 * @file
 * VuoWindowTextInternal implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoWindowTextInternal.h"
#import "VuoWindow.h"

#include <dispatch/dispatch.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindowTextInternal",
					 "dependencies" : [
						"AppKit.framework",
						"VuoWindow"
					 ]
				 });
#endif


@implementation VuoWindowTextInternal

@synthesize textView;
@synthesize scrollView;
@synthesize textFont;
@synthesize editCopyMenuItem;
@synthesize editSelectAllMenuItem;

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

		self.delegate = self;
		self.releasedWhenClosed = NO;

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

		// Remove the "Cut" option from the context menu.
		NSMenu *textViewContextMenu = [textView menu];
		int cutItemIndex = [textViewContextMenu indexOfItemWithTitle: @"Cut"];
		if (cutItemIndex >= 0)
			[textViewContextMenu removeItemAtIndex: cutItemIndex];

		// Remove the "Paste" option from the context menu.
		int pasteItemIndex = [textViewContextMenu indexOfItemWithTitle: @"Paste"];
		if (pasteItemIndex >= 0)
			[textViewContextMenu removeItemAtIndex: pasteItemIndex];

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
 * Updates the menu bar with this window's menus.
 */
- (void)becomeMainWindow
{
	[super becomeMainWindow];

	NSMenu *editMenu = [[[NSMenu alloc] initWithTitle:@"Edit"] autorelease];

	// "Edit > Copy"
	self.editCopyMenuItem = [[[NSMenuItem alloc] initWithTitle:@"Copy" action:@selector(copyText) keyEquivalent:@"c"] autorelease];
	[self.editCopyMenuItem setKeyEquivalentModifierMask:NSCommandKeyMask];
	[editMenu addItem:self.editCopyMenuItem];

	// "Edit > Select All"
	self.editSelectAllMenuItem = [[[NSMenuItem alloc] initWithTitle:@"Select All" action:@selector(selectAllText) keyEquivalent:@"a"] autorelease];
	[self.editSelectAllMenuItem setKeyEquivalentModifierMask:NSCommandKeyMask];
	[editMenu addItem:self.editSelectAllMenuItem];

	NSMenuItem *editMenuItem = [[NSMenuItem new] autorelease];
	[editMenuItem setSubmenu:editMenu];

	NSMutableArray *windowMenuItems = [NSMutableArray arrayWithCapacity:2];
	[windowMenuItems addObject:editMenuItem];
	oldMenu = [(NSMenu *)VuoApp_setMenuItems(windowMenuItems) retain];
}

/**
 * Updates the menu bar with the host app's menu prior to when this window was activated.
 */
- (void)resignMainWindow
{
	[super resignMainWindow];

	VuoApp_setMenu(oldMenu);
	[oldMenu release];
	oldMenu = nil;
}

/**
 * Updates the menu bar with the host app's menu prior to when this window was activated.
 */
- (void)windowWillClose:(NSNotification *)notification
{
	[super resignMainWindow];

	VuoApp_setMenu(oldMenu);
	[oldMenu release];
	oldMenu = nil;
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
	// Autoscroll the window only if the viewport was already at the end.
	BOOL autoscroll = abs(NSMaxY(textView.visibleRect) - NSMaxY(textView.bounds)) < 1;

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

	if (autoscroll)
		[textView scrollRangeToVisible:NSMakeRange([[textView string] length], 0)];
}

/**
 * Copies the currently selected text.
 */
- (void)copyText
{
	bool hasSelectedText = ([textView selectedRange].length > 0);
	if (hasSelectedText)
		[self.textView copy:self];
}

/**
 * Selects all text.
 */
- (void)selectAllText
{
	[self.textView selectAll:self];
}

/**
 * Decides whether or not a given user interface element should be enabled,
 * given the current state of the text window.
 */
- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem
{
	SEL theAction = [anItem action];
	if (theAction == @selector(copyText)) {

		bool hasSelectedText = ([textView selectedRange].length > 0);
		return hasSelectedText;
	}

	return [super validateUserInterfaceItem:anItem];
}

@end
