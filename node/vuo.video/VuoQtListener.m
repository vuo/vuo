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

	// VLog("kCVPixelBufferPixelFormatTypeKey  : %s", [[[pixelBufferAttributes objectForKey:(id)kCVPixelBufferPixelFormatTypeKey] stringValue] UTF8String]);
	// VLog("kCVPixelFormatName                : %s", [[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatName] stringValue] UTF8String]);
	// VLog("kCVPixelFormatConstant            : %s", [[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatConstant] stringValue] UTF8String]);
	// VLog("kCVPixelFormatCodecType           : %s", [[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatCodecType] stringValue] UTF8String]);
	// VLog("kCVPixelFormatFourCC              : %s", [[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatFourCC] stringValue] UTF8String]);
	// VLog("kCVPixelFormatBitsPerBlock        : %d", [[pixelBufferAttributes objectForKey:(id)kCVPixelFormatBitsPerBlock] integerValue]);
	// VLog("kCVPixelFormatOpenGLFormat        : %s", VuoGl_stringForConstant([[pixelBufferAttributes objectForKey:(id)kCVPixelFormatOpenGLFormat] integerValue]));
	// VLog("kCVPixelFormatOpenGLInternalFormat: %s", VuoGl_stringForConstant([[pixelBufferAttributes objectForKey:(id)kCVPixelFormatOpenGLInternalFormat] integerValue]));
	// VLog("kCVPixelFormatOpenGLType          : %s", VuoGl_stringForConstant([[pixelBufferAttributes objectForKey:(id)kCVPixelFormatOpenGLType] integerValue]));
	// VLog("kCVPixelFormatOpenGLCompatibility : %d", [[pixelBufferAttributes objectForKey:(id)kCVPixelFormatOpenGLCompatibility] integerValue]);

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
}

/**
 *	Set the callback to be triggered when a new image is available.
 */
- (void) setCallback:(void(*)(VuoVideoFrame))receivedFrame
{
	callback = receivedFrame;
}

/// Converts a FourCC (4-byte string packed into a 32-bit integer) to a null-terminated string.
#define FourCC2Str(code) (char[5]){(code >> 24) & 0xFF, (code >> 16) & 0xFF, (code >> 8) & 0xFF, code & 0xFF, 0}

/**
 *	Delegate called when a new frame is available.
 */
- (void) captureOutput:(QTCaptureOutput *)captureOutput didOutputVideoFrame:(CVImageBufferRef)imageBuffer withSampleBuffer:(QTSampleBuffer *)sampleBuffer fromConnection:(QTCaptureConnection *)connection
{
	// static bool firstRun = true;

	GLsizei texWidth    = CVPixelBufferGetWidth(imageBuffer);
	GLsizei texHeight   = CVPixelBufferGetHeight(imageBuffer);

	// If QTKit gives us a buffer with a non-square pixel aspect ratio, ask QTKit to clean up its act and try again.
	{
		NSDictionary *pixelAspect = CVBufferGetAttachment(imageBuffer, kCVImageBufferPixelAspectRatioKey, NULL);
		int pixelAspectX = [[pixelAspect objectForKey:(NSString *)kCVImageBufferPixelAspectRatioHorizontalSpacingKey] intValue];
		int pixelAspectY = [[pixelAspect objectForKey:(NSString *)kCVImageBufferPixelAspectRatioVerticalSpacingKey] intValue];
		if (pixelAspectX && pixelAspectY && ( fabs((double)pixelAspectX/(double)pixelAspectY - 1) > 0.01 ))
		{
			VLog("Got pixel aspect ratio %d:%d; requesting square.", pixelAspectX, pixelAspectY);
			NSMutableDictionary *pba = [[mCaptureDecompressedVideoOutput pixelBufferAttributes] mutableCopy];
			[pba setObject:[NSNumber numberWithInt:texWidth*pixelAspectX/pixelAspectY] forKey:(NSString *)kCVPixelBufferWidthKey];
			[pba setObject:[NSNumber numberWithInt:texHeight]                          forKey:(NSString *)kCVPixelBufferHeightKey];
			[mCaptureDecompressedVideoOutput setPixelBufferAttributes:pba];
			return;
		}
	}

	// @todo REMOVE AFTER ALPHA
	// unless it works, in which case,
	// DON'T REMOVE AFTER ALPHA
	// 
	// CVPixelBufferGetPixelFormatType() isn't responding with anything useful on cwilms computer, so attempt to figure out the color depth
	// using this round-about way.  4 means 4 bytes per pixel, or 16 bit, and vice-versa for 2 bytes (2yuv is what the Mac cam on my laptop sees)
	// int colorDepth = (CVPixelBufferGetBytesPerRow(imageBuffer) / texWidth) == 4 ? VuoImageColorDepth_16 : VuoImageColorDepth_8;

	// if(firstRun)
	// {
	// 	NSDictionary* outputPixelAttribs = (NSDictionary*) [(QTCaptureDecompressedVideoOutput*)captureOutput pixelBufferAttributes];

	// 	VLog("kCVPixelBufferPixelFormatTypeKey  : %s", [[[outputPixelAttribs objectForKey:(id)kCVPixelBufferPixelFormatTypeKey] stringValue] UTF8String]);
	// 	VLog("kCVPixelFormatName                : %s", [[[outputPixelAttribs objectForKey:(id)kCVPixelFormatName] stringValue] UTF8String]);
	// 	VLog("kCVPixelFormatConstant            : %s", [[[outputPixelAttribs objectForKey:(id)kCVPixelFormatConstant] stringValue] UTF8String]);
	// 	VLog("kCVPixelFormatCodecType           : %s", [[[outputPixelAttribs objectForKey:(id)kCVPixelFormatCodecType] stringValue] UTF8String]);
	// 	VLog("kCVPixelFormatFourCC              : %s", [[[outputPixelAttribs objectForKey:(id)kCVPixelFormatFourCC] stringValue] UTF8String]);
	// 	VLog("kCVPixelFormatBitsPerBlock        : %d", [[outputPixelAttribs objectForKey:(id)kCVPixelFormatBitsPerBlock] integerValue]);
	// 	VLog("kCVPixelFormatOpenGLFormat        : %s", VuoGl_stringForConstant([[outputPixelAttribs objectForKey:(id)kCVPixelFormatOpenGLFormat] integerValue]));
	// 	VLog("kCVPixelFormatOpenGLInternalFormat: %s", VuoGl_stringForConstant([[outputPixelAttribs objectForKey:(id)kCVPixelFormatOpenGLInternalFormat] integerValue]));
	// 	VLog("kCVPixelFormatOpenGLType          : %s", VuoGl_stringForConstant([[outputPixelAttribs objectForKey:(id)kCVPixelFormatOpenGLType] integerValue]));
	// 	VLog("kCVPixelFormatOpenGLCompatibility : %d", [[outputPixelAttribs objectForKey:(id)kCVPixelFormatOpenGLCompatibility] integerValue]);

	// 	VLog("--------------------------");
	// }

	if(callback == NULL) return;

	CVBufferRetain(imageBuffer);

	VuoImage image;

	@synchronized (self)
	{
		BOOL isFlipped = CVImageBufferIsFlipped(imageBuffer);

		// if(firstRun)
		// {
		// 	CGColorSpaceRef colorSpace = CVImageBufferGetColorSpace(imageBuffer);
		// 	CGColorSpaceModel model = CGColorSpaceGetModel(colorSpace);

		// 	if( model == kCGColorSpaceModelUnknown )
		// 		VLog("ColorSpace: kCGColorSpaceModelUnknown");
		// 	else
		// 	if( model == kCGColorSpaceModelMonochrome )
		// 		VLog("ColorSpace: kCGColorSpaceModelMonochrome");
		// 	else
		// 	if( model == kCGColorSpaceModelRGB )
		// 		VLog("ColorSpace: kCGColorSpaceModelRGB");
		// 	else
		// 	if( model == kCGColorSpaceModelCMYK )
		// 		VLog("ColorSpace: kCGColorSpaceModelCMYK");
		// 	else
		// 	if( model == kCGColorSpaceModelLab )
		// 		VLog("ColorSpace: kCGColorSpaceModelLab");
		// 	else
		// 	if( model == kCGColorSpaceModelDeviceN )
		// 		VLog("ColorSpace: kCGColorSpaceModelDeviceN");
		// 	else
		// 	if( model == kCGColorSpaceModelIndexed )
		// 		VLog("ColorSpace: kCGColorSpaceModelIndexed");
		// 	else
		// 	if( model == kCGColorSpaceModelPattern )
		// 		VLog("ColorSpace: kCGColorSpaceModelPattern");
		// 	else
		// 		VLog("ColorSpace: Unknown!!");

		// 	VLog("Frame [%s] %dx%d flipped=%d bpr=%d ds=%d (planar=%d planes=%d %dx%d bpr=%d)",
		// 		FourCC2Str(CVPixelBufferGetPixelFormatType(imageBuffer)),
		// 		texWidth, texHeight,
		// 		isFlipped,
		// 		CVPixelBufferGetBytesPerRow(imageBuffer),
		// 		CVPixelBufferGetDataSize(imageBuffer),
		// 		CVPixelBufferIsPlanar(imageBuffer),
		// 		CVPixelBufferGetPlaneCount(imageBuffer),
		// 		CVPixelBufferGetWidthOfPlane(imageBuffer, 0), CVPixelBufferGetHeightOfPlane(imageBuffer, 0),
		// 		CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, 0)
		// 	);

		// 	VLog("bytes per block: %i", CVPixelBufferGetBytesPerRow(imageBuffer) / texWidth);
		// 	VLog("color depth: %s", (colorDepth == VuoImageColorDepth_8 ? "VuoImageColorDepth_8" : "VuoImageColorDepth_16") );

		// 	size_t extraColumnsOnLeft;
		// 	size_t extraColumnsOnRight;
		// 	size_t extraRowsOnTop;
		// 	size_t extraRowsOnBottom;

		// 	CVPixelBufferGetExtendedPixels(imageBuffer, &extraColumnsOnLeft, &extraColumnsOnRight, &extraRowsOnTop, &extraRowsOnBottom );

		// 	VLog("extraColumnsOnLeft: %u extraColumnsOnRight: %u extraRowsOnTop: %u extraRowsOnBottom: %u", 
		// 		extraColumnsOnLeft,
		// 		extraColumnsOnRight,
		// 		extraRowsOnTop,
		// 		extraRowsOnBottom);
		// }

		CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

			GLvoid *baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);

			// @todo REMOVE
			// if(firstRun)
			// {
			// 	CIImage *ciImage = [CIImage imageWithCVImageBuffer:imageBuffer];
			// 	CGRect rect = [ciImage extent];

			// 	VLog("CIImage Size: %f, %f", rect.size.width, rect.size.height);

			// 	NSBitmapImageRep *newRep = [[NSBitmapImageRep alloc] initWithCIImage:ciImage];
				
			// 	NSSize size;
			// 	size.width = rect.size.width;
			// 	size.width = rect.size.height;
			// 	[newRep setSize:size];

			// 	NSData *pngData = [newRep representationUsingType:NSPNGFileType properties:nil];
			// 	[newRep autorelease];

			// 	[pngData writeToFile: [@"/tmp/Vuo-QT-Debug-Frame.png" stringByExpandingTildeInPath] atomically: YES];

			// 	firstRun = false;
			// }

			if(isFlipped)
			{
				BOOL isPlanar = CVPixelBufferIsPlanar(imageBuffer);

				if(isPlanar)
				{
					image = NULL;
				}
				else
				{
					size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
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

		CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

	}

	if(callback != NULL)
	{
		QTTime time = [sampleBuffer presentationTime];
		float second = (float)time.timeValue / (float)time.timeScale;

		callback( VuoVideoFrame_make(image, second) );
	}

	CVBufferRelease(imageBuffer);
}

/**
 *	Release instance variables.
 */
-(void) dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];

	if(mCaptureDeviceInput)
	{
		dispatch_sync(dispatch_get_main_queue(), ^{
						  [mCaptureSession removeInput:mCaptureDeviceInput];
					  });
		[[mCaptureDeviceInput device] close];
		[mCaptureDeviceInput release];
	}

	[mMovie release];
	[mCaptureSession release];

	[mCaptureDecompressedVideoOutput release];

	[mDesiredDeviceName release];
	[mDesiredDeviceID release];

	CVBufferRelease( mCurrentImageBuffer );

	[super dealloc];
}

@end
