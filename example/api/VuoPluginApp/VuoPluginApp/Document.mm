/**
 * @file
 * VuoPluginApp implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "main.hh"
#import "Document.hh"
#import <OpenGL/CGLMacro.h>
#include <OpenGL/CGLCurrent.h>

@implementation Document

/**
 * Initializes an empty VuoPluginApp document.
 */
- (id)init
{
    self = [super init];
    if (self) {
        runner = NULL;
        outputImage = NULL;
    }
    return self;
}

/**
 * Starts the running Vuo composition.
 */
- (IBAction)startRunningComposition:(id)sender {
    printf("starting\n");
    runner = VuoCompiler::newSeparateProcessRunnerFromCompositionString(compositionAsString, "/tmp");
    runner->start();
    
    [self updatePortValuesInRunningComposition];
    [self updateUI];
    
    printf("started\n");
}

/**
 * Stops the running Vuo composition.
 */
- (IBAction)stopRunningComposition:(id)sender {
    if ([self isCompositionRunning])
    {
        printf("stopping\n");
        runner->stop();
        runner->waitUntilStopped();
        delete runner;
        runner = NULL;
        printf("stopped\n");
    }
    
    [self updateUI];
}

/**
 * Responds to a change in the "time" text field value made by the user.
 */
- (IBAction)timeUpdatedFromUI:(id)sender {
    double newTime = [sender doubleValue];
    [self setTimeInRunningComposition:newTime];
}

/**
 * Responds to a change in the "image" text field value made by the user.
 */
- (IBAction)imageUpdatedFromUI:(id)sender {
    string newImageUrl = [[sender stringValue] cStringUsingEncoding:NSUTF8StringEncoding];
    
    [self setImageInRunningComposition:newImageUrl];
}

/**
 * Responds to a click of the "Set Image" button made by the user.
 */
- (IBAction)setImageButtonClicked:(id)sender {
    [self setImageInRunningComposition: [[_imageTextField stringValue] cStringUsingEncoding:NSUTF8StringEncoding]];
}

/**
 * Updates the value of the published "time" input port in the running composition
 * to the value provided.
 */
- (IBAction)setTimeInRunningComposition:(double)time {
    if (![self isCompositionRunning])
        return;
    
    VuoRunner::Port *publishedInTime = runner->getPublishedInputPortWithName("time");
    if (!publishedInTime)
        return;

   runner->setPublishedInputPortValue(publishedInTime, VuoReal_jsonFromValue(time));
   runner->firePublishedInputPortEvent(publishedInTime);
    
   [self updateOutputImage];
}

/**
 * Updates the value of the published "image" input port in the running composition
 * to the value provided.
 */
- (IBAction)setImageInRunningComposition:(string)imageUrl {
    if (![self isCompositionRunning])
        return;
    
    VuoRunner::Port *publishedInImage = runner->getPublishedInputPortWithName("image");
    if (!publishedInImage)
        return;
    
    // Convert the image file at the provided path to a VuoImage.
    NSString *imageUrlNSString = [NSString stringWithCString:imageUrl.c_str() encoding:[NSString defaultCStringEncoding]];
    NSString *file = [imageUrlNSString stringByStandardizingPath];
    NSImage *ni = [[NSImage alloc] initWithContentsOfFile:file];
    
    VuoImage vi = [self NSImageToVuoImage:ni];
    VuoRetain(vi);
	json_object *o = VuoImage_interprocessJsonFromValue(vi);
	runner->setPublishedInputPortValue(publishedInImage, o);
	json_object_put(o);
   
    runner->firePublishedInputPortEvent(publishedInImage);

    [self updateOutputImage];
}

/**
 * Updates the outputImage displayed in the UI to the current value present in
 * the running composition.
 */
- (void)updateOutputImage
{
    // Get the VuoImage from the running composition
    VuoRunner::Port *publishedOutImage = runner->getPublishedOutputPortWithName("outputImage");
    if (!publishedOutImage)
        return;
    
    runner->waitForAnyPublishedOutputPortEvent();
    json_object *outputImageJson = runner->getPublishedOutputPortValue(publishedOutImage);
    
    VuoImage vi = VuoImage_valueFromJson(outputImageJson);
    json_object_put(outputImageJson);
    
    if (!vi)
        return;
    
    // Display the image
    VuoRetain(vi);
    NSImage *ni = [self VuoImageToNSImage:vi];
    [_outputImageView setImage:ni];
    VuoRelease(vi);
}

/**
 * Sent after the specified window controller loads a nib file if the receiver is the nib file’s owner.
 */
- (void)windowControllerDidLoadNib:(NSWindowController *)aController
{
    [super windowControllerDidLoadNib:aController];
    
    // Add any code here that needs to be executed once the windowController has loaded the document's window.
    runner = VuoCompiler::newSeparateProcessRunnerFromCompositionString(compositionAsString, "/tmp");
    runner->startPaused();
    
    [self validatePublishedPorts];
    [self stopRunningComposition:0];
    [self initializeDefaultPortValuesInUI];
    [self updateUI];
}

/**
 * Initializes default port values in the UI.
 */
- (void)initializeDefaultPortValuesInUI
{
    NSString *defaultImagePath = @"/System/Library/Automator/Send Birthday Greetings.action/Contents/Resources/4.jpg";
    double defaultTime = 3.14;
    
    [_imageTextField setStringValue:defaultImagePath];
    [_timeSlider setDoubleValue:defaultTime];
}

/**
 * Updates the published port values in the running composition to conform to the values
 * currently displayed in the UI.
 */
- (void)updatePortValuesInRunningComposition
{
    [self setImageInRunningComposition:[[_imageTextField stringValue] cStringUsingEncoding:NSUTF8StringEncoding]];
    [self setTimeInRunningComposition:[_timeSlider doubleValue]];
}

/**
 * Updates the UI elements (e.g., enables/disables buttons) based on the application's state.
 */
- (void)updateUI
{
    bool compositionRunning = [self isCompositionRunning];
    
    [_runButton setEnabled: !compositionRunning];
    [_stopButton setEnabled: compositionRunning];
}

/**
 * Returns a boolean indicating whether the composition is currently running.
 */
- (bool)isCompositionRunning
{
    return (runner && !runner->isStopped());
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
- (BOOL)readFromData:(NSData *)data ofType:(NSString *)typeName error:(NSError **)outError
{
    if ([typeName compare:@"vuoComposition"] != NSOrderedSame)
    {
        NSLog(@"** ERROR ** readFromData pTypeName=%@",typeName);
        *outError = [NSError errorWithDomain:NSOSStatusErrorDomain
                                         code:unimpErr
                                     userInfo:NULL];
        return NO;
    }
    
    NSLog(@"readFromData file type OK");
    
    
    NSDictionary *zDict = [NSDictionary dictionaryWithObjectsAndKeys:
                           NSPlainTextDocumentType,
                           NSDocumentTypeDocumentAttribute,
                           nil];
    NSDictionary *zDictDocAttributes;
    NSError *zError = nil;
    NSAttributedString * zNSAttributedStringObj =
    [[NSAttributedString alloc]initWithData:data
                                    options:zDict
                         documentAttributes:&zDictDocAttributes
                                      error:&zError];
    
    NSString *compositionAsNSString = [zNSAttributedStringObj string];
    compositionAsString = ([compositionAsNSString cStringUsingEncoding:NSUTF8StringEncoding]);
    
    if ( zError != NULL )
    {
        NSLog(@"Error readFromData: %@",[zError localizedDescription]);
        return NO;
    }

    return YES;
}

/**
 * Checks whether the loaded composition contains the expected published ports, and
 * disables any UI interface elements associated with publihed ports found to be missing.
 */
- (void)validatePublishedPorts
{
    // Expected published input: "time"
    VuoRunner::Port *publishedInTime = runner->getPublishedInputPortWithName("time");
    
    if (!publishedInTime)
    {
        [_timeLabel setTextColor:[NSColor redColor]];
        [_timeSlider setEnabled:NO];
    }
    
    // Expected published input: "image"
    VuoRunner::Port *publishedInImage = runner->getPublishedInputPortWithName("image");
    
    if (!publishedInImage)
    {
        [_imageLabel setTextColor:[NSColor redColor]];
        [_imageTextField setEnabled:NO];
        [_setImageButton setEnabled:NO];
    }
    
    // Expected published output: "outputImage"
    VuoRunner::Port *publishedOutImage = runner->getPublishedOutputPortWithName("outputImage");
    
    if (!publishedOutImage)
    {
        [_outputImageLabel setTextColor:[NSColor redColor]];
        [_outputImageView setEnabled:NO];
    }
}

/**
 * Converts the provided VuoImage to an NSImage*.
 */
- (NSImage*)VuoImageToNSImage:(VuoImage)vi
{
	// Allocate memory to store the image data.
	NSBitmapImageRep *nbi = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
		pixelsWide:vi->pixelsWide
		pixelsHigh:vi->pixelsHigh
		bitsPerSample:8
		samplesPerPixel:4
		hasAlpha:YES
		isPlanar:NO
		colorSpaceName:NSDeviceRGBColorSpace
		bytesPerRow:4*vi->pixelsWide
		bitsPerPixel:0];

	// Download the image data from the GPU.
	{
		CGLContextObj cgl_ctx = (CGLContextObj)[VuoPluginAppSharedOpenGLContext CGLContextObj];
		glBindTexture(GL_TEXTURE_2D, vi->glTextureName);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, [nbi bitmapData]);
	}

	NSImage *ni = [ [NSImage alloc] initWithSize:[nbi size]];
	[ni setFlipped:YES];
	[ni addRepresentation:nbi];
	return ni;
}

/**
 * Converts the provided NSImage* to a VuoImage.
 */
- (VuoImage)NSImageToVuoImage:(NSImage *)ni
{
	// Load image into a GL Texture
    CGLContextObj cgl_ctx = (CGLContextObj)[VuoPluginAppSharedOpenGLContext CGLContextObj];
    
    GLuint inputTexture;
    glGenTextures(1, &inputTexture);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    
	NSImage *niFlipped = [[NSImage alloc] initWithSize:[ni size]];
	float scale = 1;
	if ([niFlipped respondsToSelector:@selector(recommendedLayerContentsScale:)])
	{
		// If we're on 10.7 or later, we need to check whether we're running on a retina display, and scale accordingly if so.
		typedef CGFloat (*funcType)(id receiver, SEL selector, CGFloat);
		funcType recommendedLayerContentsScale = (funcType)[[niFlipped class] instanceMethodForSelector:@selector(recommendedLayerContentsScale:)];
		scale = recommendedLayerContentsScale(niFlipped, @selector(recommendedLayerContentsScale:), 0);
	}
	[niFlipped setFlipped:YES];
	[niFlipped lockFocus];
	[ni drawInRect:NSMakeRect(0,0,[ni size].width,[ni size].height) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
	[niFlipped unlockFocus];
	NSBitmapImageRep *nbir = [NSBitmapImageRep imageRepWithData:[niFlipped TIFFRepresentation]];
	GLenum internalformat = GL_RGBA;
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, [ni size].width*scale, [ni size].height*scale, 0, GL_RGBA, GL_UNSIGNED_BYTE, [nbir bitmapData]);
    
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    
    VuoImage vi = VuoImage_make(inputTexture, internalformat, [ni size].width, [ni size].height);
	return vi;
}

/**
 * Tells the delegate that the window is about to close.
 */
- windowWillClose:sender
{
    // Stop the running composition associated with this window.
    [self stopRunningComposition:0];
    
    return self;
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
