/**
 * @file
 * VuoQtListener implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoQtListener.h"
#include "VuoImageRenderer.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
#include <QuartzCore/CoreImage.h>
#include "module.h"
#include "VuoGlPool.h"
#include <Quartz/Quartz.h>
#include <QuartzCore/CVImageBuffer.h>
#include "VuoVideoFrame.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoQtListener",
					 "dependencies" : [
						"VuoImage",
						"VuoImageRenderer",
						"QTKit.framework",
						"QuartzCore.framework",
						"Quartz.framework",
						"VuoGLContext"
					 ]
				 });
#endif

@implementation VuoQtListener

/**
 * Initializes for listening to a specific device.

 */
- (BOOL) initWithDevice:(NSString*)name id:(NSString*)uniqueID callback:(void(*)(VuoVideoFrame))receivedFrame
{
	NSError *error;

	mCaptureSession = [[QTCaptureSession alloc] init];
	mMovie = [[QTMovie alloc] initToWritableData:[NSMutableData data] error:&error];

	if (!mMovie) {
		VLog("Failed initializing QTMovie");
		return FALSE;
	}

	[self setInputDevice:name id:uniqueID];

	mCaptureDecompressedVideoOutput = [[QTCaptureDecompressedVideoOutput alloc] init];

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(devicesDidChange:) name:QTCaptureDeviceWasConnectedNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(devicesDidChange:) name:QTCaptureDeviceWasDisconnectedNotification object:nil];

	// https://developer.apple.com/library/mac/documentation/quicktime/reference/QTCaptureDecompressedVideoOutput_Ref/DeprecationAppendix/AppendixADeprecatedAPI.html#//apple_ref/occ/instm/QTCaptureDecompressedVideoOutput/pixelBufferAttributes
	// https://developer.apple.com/library/mac/qa/qa1501/_index.html

	NSDictionary *pixelBufferAttributes = (NSDictionary*)CVPixelFormatDescriptionCreateWithPixelFormatType(kCFAllocatorDefault, k32ARGBPixelFormat);

	// VLog("bits per block: %d", [[pixelBufferAttributes objectForKey:(id)kCVPixelFormatBitsPerBlock] integerValue]);
	// VLog("gl format: %s", [[VuoQtListener stringWithGlFormat:(GLenum)[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatOpenGLFormat] integerValue]] UTF8String] );
	// VLog("gl internalformat: %s", [[VuoQtListener stringWithGlFormat:(GLenum)[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatOpenGLInternalFormat] integerValue]] UTF8String] );
	// VLog("gl type: %s", [[VuoQtListener stringWithGlType:(GLenum)[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatOpenGLType] integerValue]] UTF8String] );
	// VLog("gl compatible: %s", [pixelBufferAttributes objectForKey:(id)kCVPixelFormatOpenGLCompatibility] ? "yes" : "no");

	/*
		CFShow(pixelBufferAttributes) -> k32ARGBPixelFormat attributes

		"CGImageCompatibility"			= <CFBoolean> 		{value = true}
		"FillExtendedPixelsCallback"	= <CFData> 			{length = 24, capacity = 24, bytes = 0x0000000000000000820b0d8cff7f00000000000000000000}
		"CGBitmapContextCompatibility"	= <CFBoolean> 		{value = true}
		"OpenGLFormat"					= <CFNumber> 		{value = +32993, 	type = kCFNumberSInt32Type}
		"BitsPerBlock"					= <CFNumber> 		{value = +32, 		type = kCFNumberSInt32Type}
		"BlackBlock"					= <CFData> 			{length = 4, capacity = 4, bytes = 0xff000000}
		"OpenGLInternalFormat"			= <CFNumber> 		{value = +32856, 	type = kCFNumberSInt32Type}
		"ContainsAlpha"					= <CFBoolean> 		{value = true}
		"CGBitmapInfo"					= <CFNumber> 		{value = +16388, 	type = kCFNumberSInt32Type}
		"OpenGLCompatibility"			= <CFBoolean> 		{value = true}
		"PixelFormat"					= <CFNumber> 		{value = +32, 		type = kCFNumberSInt32Type}
		"OpenGLType"					= <CFNumber> 		{value = +32821, 	type = kCFNumberSInt32Type}
		"QDCompatibility"				= <CFBoolean> 		{value = true}
	*/

	[mCaptureDecompressedVideoOutput setPixelBufferAttributes:pixelBufferAttributes];

	[mCaptureDecompressedVideoOutput setDelegate:self];

	BOOL success = [mCaptureSession addOutput:mCaptureDecompressedVideoOutput error:&error];

	if (!success)
	{
		VLog("Failed creating a video output for QTCaptureSession.");
		return FALSE;
	}

	callback = receivedFrame;

	return TRUE;
}

/**
 *	Set the input device - does not start or stop recording, just changes what the input
 *	device is.
 */
- (void) setInputDevice:(NSString*)name id:(NSString*)uniqueID
{
	if(mDesiredDeviceName != nil) [mDesiredDeviceName release];
	if(mDesiredDeviceID != nil) [mDesiredDeviceID release];

	mDesiredDeviceName = name == nil ? @"" : name;	// Don't allow null strings
	[mDesiredDeviceName retain];

	mDesiredDeviceID = uniqueID == nil ? @"" : uniqueID;
	[mDesiredDeviceID retain];

	[self refreshDevices];

	// In -captureOutput:…, the buffer size may be forced to correct for non-square pixel aspect ratio.
	// Undo it, so we can get the raw (unresized) buffer if possible.
	// @todo This hangs unless it executes on the main queue..?
	dispatch_sync(dispatch_get_main_queue(), ^{
					  NSMutableDictionary *pba = [[mCaptureDecompressedVideoOutput pixelBufferAttributes] mutableCopy];
					  [pba removeObjectForKey:(NSString *)kCVPixelBufferWidthKey];
					  [pba removeObjectForKey:(NSString *)kCVPixelBufferHeightKey];
					  [mCaptureDecompressedVideoOutput setPixelBufferAttributes:pba];
				  });
}

/**
 *	When a device is connected or disconnected, check if the device we're interested in has been found or lost.
 */
- (void) devicesDidChange:(NSNotification *)notification
{
	[self refreshDevices];
}

NSString *VuoQTCapture_getVendorNameForUniqueID(NSString *uniqueID);

/**
 *	Iterate through available input devices and search for one matching the desired device name or id, then set
 *	the input device (but don't start recording - only do that in startListening).  If both name and id are
 *	empty strings, the first valid device is used.  If name or id is set, a nil device will be set as the input
 *	until a matching device is detected.
 */
- (void) refreshDevices
{
	if(mVideoInputDevices)
		[mVideoInputDevices release];

	mVideoInputDevices = [[[QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo] arrayByAddingObjectsFromArray:[QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeMuxed]] retain];

	// If looking for a specific device, make sure to call setCaptureDevice, even if device == null.
	BOOL lookingForDevice = [mDesiredDeviceName length] != 0 || [mDesiredDeviceID length] != 0;

	QTCaptureDevice *device;

	// First check if this is an empty string, and if so, just grab the first available video feed.
	if(!lookingForDevice)
	{
		device = [QTCaptureDevice defaultInputDeviceWithMediaType:QTMediaTypeVideo];
	}
	else
	{
		device = [QTCaptureDevice deviceWithUniqueID:mDesiredDeviceID];

		// If that fails, try to match the device name
		if(!device || ![device isConnected])
		{
			for(QTCaptureDevice *qd in mVideoInputDevices)
			{
				NSString *deviceName = [NSString stringWithFormat:@"%@ %@", VuoQTCapture_getVendorNameForUniqueID([qd uniqueID]), [qd localizedDisplayName]];
				if ([deviceName rangeOfString:mDesiredDeviceName].location != NSNotFound)
				{
					device = qd;
					break;
				}
			}
		}
	}

	if( mCaptureDeviceInput && device == [mCaptureDeviceInput device] )
	{
		// The currently running device is already correct - do nothing.
		return;
	}
	else
	{
		// Device may be nil - allow it.
		[self setCaptureDevice:device];
	}
}

/**
 *	Set the capture device.
 */
- (void) setCaptureDevice:(QTCaptureDevice*)device
{
	if(mCaptureDeviceInput)
	{
		// If the device is changed to a non-default device on startup
		// (e.g., by the initial event from `List Video Devices`),
		// it hangs unless it executes on the main queue.
		dispatch_sync(dispatch_get_main_queue(), ^{
						  [mCaptureSession removeInput:mCaptureDeviceInput];
					  });
		[[mCaptureDeviceInput device] close];
		[mCaptureDeviceInput release];
		mCaptureDeviceInput = nil;
	}

	if(device)
	{
		NSError *error = nil;
		BOOL success = [device open:&error];

		if (!success)
			VLog("Failed initializing QTCaptureDevice: %s", [[device localizedDisplayName] UTF8String]);

		mCaptureDeviceInput = [[QTCaptureDeviceInput alloc] initWithDevice:device];

		success = [mCaptureSession addInput:mCaptureDeviceInput error:&error];

		if (!success)
		{
			[mCaptureDeviceInput release];
			mCaptureDeviceInput = nil;
		}
		else
		{
			if(userWantsRunning && ![self isRunning])
			{
				[self startRunning];
			}
		}
	}
}

/**
 * Returns true if the capture session has been initialized.  Does not reflect the status
 * of capture session isRun
 */
- (BOOL) isInitialized
{
	return mCaptureSession != NULL;
}

/**
 *	Reflects the actual running status of mCaptureSession, not whether or not the user
 *	wants isRunning.
 */
- (BOOL) isRunning
{
	return [mCaptureSession isRunning];
}

/**
 * Begins receiving frames.
 */
- (void) startRunning
{
	userWantsRunning = YES;

	QTCaptureDevice *device = [mCaptureDeviceInput device];

	NSError *error;
	BOOL success = [device open:&error];
	if (!success) {
		return;
	}

	[mMovie setCurrentTime: QTMakeTime(0, 1)];

	[mCaptureSession startRunning];
}

/**
 * Stops receiving frames (starts ignoring/dropping them).
 */
- (void) stopRunning
{
	userWantsRunning = NO;

	[mCaptureSession stopRunning];

	QTCaptureDevice *device = [mCaptureDeviceInput device];

	if(device == nil)
		return;

	if([device isOpen])
		[device close];

	[device release];
	device = nil;
}

/**
 *	Set the callback to be triggered when a new image is available.
 */
- (void) setCallback:(void(*)(VuoVideoFrame))receivedFrame
{
	callback = receivedFrame;
}

/**
 *	Delegate called when a new frame is available.
 */
- (void) captureOutput:(QTCaptureOutput *)captureOutput didOutputVideoFrame:(CVImageBufferRef)videoFrame withSampleBuffer:(QTSampleBuffer *)sampleBuffer fromConnection:(QTCaptureConnection *)connection
{
	GLsizei texWidth    = CVPixelBufferGetWidth(videoFrame);
	GLsizei texHeight   = CVPixelBufferGetHeight(videoFrame);

	// If QTKit gives us a buffer with a non-square pixel aspect ratio, ask QTKit to clean up its act and try again.
	{
		NSDictionary *pixelAspect = CVBufferGetAttachment(videoFrame, kCVImageBufferPixelAspectRatioKey, NULL);
		int pixelAspectX = [[pixelAspect objectForKey:(NSString *)kCVImageBufferPixelAspectRatioHorizontalSpacingKey] intValue];
		int pixelAspectY = [[pixelAspect objectForKey:(NSString *)kCVImageBufferPixelAspectRatioVerticalSpacingKey] intValue];
		if (pixelAspectX && pixelAspectY && ( fabs((double)pixelAspectX/(double)pixelAspectY - 1) > 0.01 ))
		{
			NSMutableDictionary *pba = [[mCaptureDecompressedVideoOutput pixelBufferAttributes] mutableCopy];
			[pba setObject:[NSNumber numberWithInt:texWidth*pixelAspectX/pixelAspectY] forKey:(NSString *)kCVPixelBufferWidthKey];
			[pba setObject:[NSNumber numberWithInt:texHeight]                          forKey:(NSString *)kCVPixelBufferHeightKey];
			[mCaptureDecompressedVideoOutput setPixelBufferAttributes:pba];
			return;
		}
	}

	if(callback == NULL) return;

	CVBufferRetain(videoFrame);

	VuoImage image;

	@synchronized (self)
	{
		BOOL isFlipped = CVImageBufferIsFlipped(videoFrame);

		CVPixelBufferLockBaseAddress(videoFrame, kCVPixelBufferLock_ReadOnly);


			GLvoid *baseAddress = CVPixelBufferGetBaseAddress(videoFrame);

			if(isFlipped)
			{
				BOOL isPlanar = CVPixelBufferIsPlanar(videoFrame);

				if(isPlanar)
				{
					image = NULL;
				}
				else
				{
					size_t bytesPerRow = CVPixelBufferGetBytesPerRow(videoFrame);
					char *tmp = (char*)malloc(bytesPerRow * texHeight);

					for(int i = 0; i < texHeight; i++)
					{
						void* start = tmp + bytesPerRow * (texHeight-i-1);
						memcpy(start, baseAddress + (bytesPerRow * i), bytesPerRow);
					}

					image = VuoImage_makeFromBuffer(tmp, GL_YCBCR_422_APPLE, texWidth, texHeight, VuoImageColorDepth_8);

					free(tmp);
				}
			}
			else
			{
				image = VuoImage_makeFromBuffer(baseAddress, GL_YCBCR_422_APPLE, texWidth, texHeight, VuoImageColorDepth_8);
			}

		CVPixelBufferUnlockBaseAddress(videoFrame, kCVPixelBufferLock_ReadOnly);

	}

	if(callback != NULL)
	{
		QTTime time = [sampleBuffer presentationTime];
		float second = (float)time.timeValue / (float)time.timeScale;

		callback( VuoVideoFrame_make(image, second) );
	}

	CVBufferRelease(videoFrame);
}

/**
 *	Release instance variables.
 */
-(void) dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];

	[mMovie release];
	[mCaptureSession release];
	if(mCaptureDeviceInput)
		[mCaptureDeviceInput release];
	[mCaptureDecompressedVideoOutput release];

	[mDesiredDeviceName release];
	[mDesiredDeviceID release];

	CVBufferRelease( mCurrentImageBuffer );

	[super dealloc];
}

@end
