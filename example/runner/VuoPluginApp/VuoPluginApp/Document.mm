/**
 * @file
 * VuoPluginApp implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "main.hh"
#import "Document.hh"
#import <OpenGL/CGLMacro.h>
#include <OpenGL/CGLCurrent.h>

@implementation Document
@synthesize setImageButton = _setImageButton;
@synthesize timeSlider = _timeSlider;
@synthesize outputImageView = _outputImageView;
@synthesize imageTextField = _imageTextField;
@synthesize imageLabel = _imageLabel;
@synthesize timeLabel = _timeLabel;
@synthesize outputImageLabel = _outputImageLabel;

/**
 * Initializes an empty VuoPluginApp document.
 */
- (id)init
{
    self = [super init];
    if (self) {
		time = 0;
    }
    return self;
}

/**
 * Executes the image filter, and updates the image displayed in the UI.
 */
- (void)updateOutputImage
{
	NSImage *filteredImage = [imageFilter filterNSImage:sourceImage atTime:time];
	[_outputImageView setImage:filteredImage];
}

/**
 * Loads the specified image.
 */
- (IBAction)setImageInRunningComposition:(NSString *)imagePath {
	sourceImage = [[NSImage alloc] initWithContentsOfFile:[imagePath stringByStandardizingPath]];
	if (!sourceImage)
	{
		NSAlert *a = [NSAlert new];
		[a setMessageText:@"Unable to read image."];
		[a setInformativeText:imagePath];
		NSWindow *w = [[[self windowControllers] objectAtIndex:0] window];
		[a beginSheetModalForWindow:w completionHandler:^(NSInteger returnCode){}];
		[a release];
		return;
	}
	
    [self updateOutputImage];
}

/**
 * Responds to a change in the "time" text field value made by the user.
 */
- (IBAction)timeUpdatedFromUI:(id)sender {
	time = [sender doubleValue];
	[self updateOutputImage];
}

/**
 * Responds to a change in the "image" text field value made by the user.
 */
- (IBAction)imageUpdatedFromUI:(id)sender {
	[self setImageInRunningComposition:[sender stringValue]];
}

/**
 * Responds to a click of the "Set Image" button made by the user.
 */
- (IBAction)setImageButtonClicked:(id)sender {
	[self setImageInRunningComposition:[_imageTextField stringValue]];
}

/**
 * Sent after the specified window controller loads a nib file if the receiver is the nib file’s owner.
 */
- (void)windowControllerDidLoadNib:(NSWindowController *)aController
{
    [super windowControllerDidLoadNib:aController];
    
    // Add any code here that needs to be executed once the windowController has loaded the document's window.
	
	[_imageTextField setStringValue:@"/System/Library/Automator/Send Birthday Greetings.action/Contents/Resources/3.jpg"];
	[self setImageButtonClicked:nil];
}

/**
 * Creates and returns a data object that contains the contents of the document, formatted to
 * a specified type.
 */
- (NSData *)dataOfType:(NSString *)typeName error:(NSError **)outError
{
    NSException *exception = [NSException exceptionWithName:@"UnimplementedMethod" reason:[NSString stringWithFormat:@"%@ is unimplemented", NSStringFromSelector(_cmd)] userInfo:nil];
    @throw exception;
    return nil;
}

/**
 * Sets the contents of this document by reading from data of a specified type and returns YES if
 * successful.
 */
- (BOOL)readFromURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError
{
	imageFilter = [[VuoImageFilter alloc] initWithComposition:absoluteURL];

	if (!imageFilter)
	{
		*outError = [NSError errorWithDomain:NSCocoaErrorDomain code:NSFileReadUnknownError userInfo:nil];
		return NO;
	}

	return YES;
}

/**
 * Tells the delegate that the window is about to close.
 */
- (void)windowWillClose:(NSNotification *)sender
{
    // Stop the running composition associated with this window.
	[imageFilter release];
}

/**
 * Returns the name of the document’s sole nib file.
 */
- (NSString *)windowNibName
{
    return @"Document";
}

/**
 * Returns whether the receiver supports autosaving in place.
 */
+ (BOOL)autosavesInPlace
{
    return YES;
}

@end
