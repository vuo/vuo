/**
 * @file
 * VuoQtListener implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
#include "VuoWindow.h"

#ifndef DOXYGEN
// Exists on 10.7 but is inexplicably missing from the CoreVideo header.
CV_EXPORT Boolean CVImageBufferIsFlipped(CVImageBufferRef imageBuffer) __OSX_AVAILABLE_STARTING(__MAC_10_4,__IPHONE_4_0);
#endif

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoQtListener",
					 "dependencies" : [
						"VuoImage",
						"VuoImageRenderer",
						"VuoWindow",
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
		VUserLog("Failed initializing QTMovie");
		return FALSE;
	}

	/// initialize texture cache for creating gl textures from CVPixelBuffer
	glContext = VuoGlContext_use();
	CGLPixelFormatObj pf = VuoGlContext_makePlatformPixelFormat(false);
	CVReturn ret = CVOpenGLTextureCacheCreate(NULL, NULL, (CGLContextObj)glContext, pf, NULL, &textureCache);
	CGLReleasePixelFormat(pf);
	if (ret != kCVReturnSuccess)
	{
		VUserLog("Error: Couldn't create texture cache: %d", ret);
		return false;
	}

	[self setInputDevice:name id:uniqueID];

	mCaptureDecompressedVideoOutput = [[QTCaptureDecompressedVideoOutput alloc] init];

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(devicesDidChange:) name:QTCaptureDeviceWasConnectedNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(devicesDidChange:) name:QTCaptureDeviceWasDisconnectedNotification object:nil];

	// https://developer.apple.com/library/mac/documentation/quicktime/reference/QTCaptureDecompressedVideoOutput_Ref/DeprecationAppendix/AppendixADeprecatedAPI.html#//apple_ref/occ/instm/QTCaptureDecompressedVideoOutput/pixelBufferAttributes
	// https://developer.apple.com/library/mac/qa/qa1501/_index.html

	NSDictionary *pixelBufferAttributes = nil;

	// @todo  https://b33p.net/kosada/node/10194
	SInt32 macMinorVersion;
	Gestalt(gestaltSystemVersionMinor, &macMinorVersion);

	if(VuoIsDebugEnabled())
	{
		VUserLog("Available Pixel Formats Usage: `defaults write org.vuo.Editor liveVideoPixelFormat 846624121`.  This examples sets the pixel format to kCVPixelFormatType_422YpCbCr8. See https://developer.apple.com/library/mac/documentation/QuartzCore/Reference/CVPixelFormatDescriptionRef/#//apple_ref/doc/constant_group/Pixel_Format_Types");

		CFArrayRef formats = CVPixelFormatDescriptionArrayCreateWithAllPixelFormatTypes(kCFAllocatorDefault);
		for(CFIndex i = 0; i < CFArrayGetCount(formats); i++)
		{
			// CFStringRef pixelFormat = NULL;
			CFNumberRef pixelFormatFourCC = (CFNumberRef) CFArrayGetValueAtIndex(formats, i);

			if(pixelFormatFourCC != NULL)
			{
				UInt32 value = 0;
				CFNumberGetValue(pixelFormatFourCC, kCFNumberSInt32Type, &value);
				VUserLog("%-64s (FourCC): %c%c%c%c key: %d", [VuoQtListener formatTypeString:value], (char)(value >> 24), (char)(value >> 16), (char)(value >> 8), (char)value, value);
			}
		}

	}
	Boolean overridden = false;

	/// https://developer.apple.com/library/mac/documentation/QuartzCore/Reference/CVPixelFormatDescriptionRef/#//apple_ref/doc/constant_group/Pixel_Format_Types
	int pixelFormat = CFPreferencesGetAppIntegerValue(CFSTR("liveVideoPixelFormat"), CFSTR("org.vuo.Editor"), &overridden);

	if(overridden)
	{
		VUserLog("User set pixel format type %s (fourcc): %c%c%c%c   key: %i", [VuoQtListener formatTypeString:pixelFormat], (char)(pixelFormat >> 24), (char)(pixelFormat >> 16), (char)(pixelFormat >> 8), (char)pixelFormat, pixelFormat);
		pixelBufferAttributes = (NSDictionary*)CVPixelFormatDescriptionCreateWithPixelFormatType(kCFAllocatorDefault, pixelFormat);
	}
	else
	{
		if (macMinorVersion != 8)
			pixelBufferAttributes = (NSDictionary*)CVPixelFormatDescriptionCreateWithPixelFormatType(kCFAllocatorDefault, kCVPixelFormatType_422YpCbCr8);
		else
			pixelBufferAttributes = (NSDictionary*)CVPixelFormatDescriptionCreateWithPixelFormatType(kCFAllocatorDefault, kCVPixelFormatType_32ARGB);
	}


	if (VuoIsDebugEnabled())
	{
		VUserLog("Asked Qt for pixel format attributes:");
		VUserLog("\tkCVPixelBufferPixelFormatTypeKey  : %s", [[[pixelBufferAttributes objectForKey:(id)kCVPixelBufferPixelFormatTypeKey] stringValue] UTF8String]);
		VUserLog("\tkCVPixelFormatName                : %s", [[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatName] stringValue] UTF8String]);
		VUserLog("\tkCVPixelFormatConstant            : %s", [[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatConstant] stringValue] UTF8String]);
		VUserLog("\tkCVPixelFormatCodecType           : %s", [[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatCodecType] stringValue] UTF8String]);
		VUserLog("\tkCVPixelFormatFourCC              : %s", [[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatFourCC] stringValue] UTF8String]);
		VUserLog("\tkCVPixelFormatBitsPerBlock        : %ld",[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatBitsPerBlock] integerValue]);
		VUserLog("\tkCVPixelFormatOpenGLFormat        : %s", VuoGl_stringForConstant([[pixelBufferAttributes objectForKey:(id)kCVPixelFormatOpenGLFormat] integerValue]));
		VUserLog("\tkCVPixelFormatOpenGLInternalFormat: %s", VuoGl_stringForConstant([[pixelBufferAttributes objectForKey:(id)kCVPixelFormatOpenGLInternalFormat] integerValue]));
		VUserLog("\tkCVPixelFormatOpenGLType          : %s", VuoGl_stringForConstant([[pixelBufferAttributes objectForKey:(id)kCVPixelFormatOpenGLType] integerValue]));
		VUserLog("\tkCVPixelFormatOpenGLCompatibility : %ld",[[pixelBufferAttributes objectForKey:(id)kCVPixelFormatOpenGLCompatibility] integerValue]);
	}

	[mCaptureDecompressedVideoOutput setPixelBufferAttributes:pixelBufferAttributes];

	[mCaptureDecompressedVideoOutput setDelegate:self];

	BOOL success = [mCaptureSession addOutput:mCaptureDecompressedVideoOutput error:&error];

	if (!success)
	{
		VUserLog("Failed creating a video output for QTCaptureSession.");
		return FALSE;
	}

	callback = receivedFrame;

	return TRUE;
}

/**
 *	Release instance variables.
 */
-(void) dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];

	// Disable the delegate, since it might still be trying to invoke the -captureOutput:… callback.
	[mCaptureDecompressedVideoOutput setDelegate:nil];

	if(mCaptureDeviceInput)
	{
		VUOLOG_PROFILE_BEGIN(mainQueue);
		dispatch_sync(dispatch_get_main_queue(), ^{
						  VUOLOG_PROFILE_END(mainQueue);
						  [mCaptureSession removeInput:mCaptureDeviceInput];
					  });
		[[mCaptureDeviceInput device] close];
		[mCaptureDeviceInput release];
	}

	CVOpenGLTextureCacheRelease(textureCache);
	VuoGlContext_disuse(glContext);

	[mMovie release];
	[mCaptureSession release];

	[mCaptureDecompressedVideoOutput release];

	[mDesiredDeviceName release];
	[mDesiredDeviceID release];

	CVBufferRelease( mCurrentImageBuffer );

	[super dealloc];
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
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
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
		if (VuoApp_isMainThread())
			[mCaptureSession removeInput:mCaptureDeviceInput];
		else
		{
			// If the device is changed to a non-default device on startup
			// (e.g., by the initial event from `List Video Devices`),
			// it hangs unless it executes on the main queue.
			VUOLOG_PROFILE_BEGIN(mainQueue);
			dispatch_sync(dispatch_get_main_queue(), ^{
						  VUOLOG_PROFILE_END(mainQueue);
						  [mCaptureSession removeInput:mCaptureDeviceInput];
					  });
		}
		[[mCaptureDeviceInput device] close];
		[mCaptureDeviceInput release];
		mCaptureDeviceInput = nil;
	}

	if(device)
	{
		NSError *error = nil;
		BOOL success = [device open:&error];

		if (!success)
		{
			VUserLog("Couldn't initialize \"%s\": %s", [[device localizedDisplayName] UTF8String], [[error localizedDescription] UTF8String]);
			mCaptureDeviceInput = nil;
			return;
		}

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

	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
		VUOLOG_PROFILE_END(mainQueue);
		[mCaptureSession stopRunning];
	});
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
 * Does nothing.
 */
static void VuoAvPlayerObject_freeCallback(VuoImage imageToFree)
{
}

/**
 *	Delegate called when a new frame is available.
 */
- (void) captureOutput:(QTCaptureOutput *)captureOutput didOutputVideoFrame:(CVImageBufferRef)imageBuffer withSampleBuffer:(QTSampleBuffer *)sampleBuffer fromConnection:(QTCaptureConnection *)connection
{
	static bool firstRun = true;

	GLsizei texWidth    = CVPixelBufferGetWidth(imageBuffer);
	GLsizei texHeight   = CVPixelBufferGetHeight(imageBuffer);

	// If QTKit gives us a buffer with a non-square pixel aspect ratio, ask QTKit to clean up its act and try again.
	{
		NSDictionary *pixelAspect = CVBufferGetAttachment(imageBuffer, kCVImageBufferPixelAspectRatioKey, NULL);
		int pixelAspectX = [[pixelAspect objectForKey:(NSString *)kCVImageBufferPixelAspectRatioHorizontalSpacingKey] intValue];
		int pixelAspectY = [[pixelAspect objectForKey:(NSString *)kCVImageBufferPixelAspectRatioVerticalSpacingKey] intValue];
		if (pixelAspectX && pixelAspectY && ( fabs((double)pixelAspectX/(double)pixelAspectY - 1) > 0.01 ))
		{
			VUserLog("Got pixel aspect ratio %d:%d; requesting square.", pixelAspectX, pixelAspectY);
			NSMutableDictionary *pba = [[mCaptureDecompressedVideoOutput pixelBufferAttributes] mutableCopy];
			[pba setObject:[NSNumber numberWithInt:texWidth*pixelAspectX/pixelAspectY] forKey:(NSString *)kCVPixelBufferWidthKey];
			[pba setObject:[NSNumber numberWithInt:texHeight]                          forKey:(NSString *)kCVPixelBufferHeightKey];
			[mCaptureDecompressedVideoOutput setPixelBufferAttributes:pba];
			return;
		}
	}

	NSDictionary* outputPixelAttribs = (NSDictionary*) [(QTCaptureDecompressedVideoOutput*)captureOutput pixelBufferAttributes];

	if (firstRun && VuoIsDebugEnabled())
	{
		VUserLog("Qt actually delivered pixel format attributes:");
		VUserLog("kCVPixelBufferPixelFormatTypeKey  : %s", [[[outputPixelAttribs objectForKey:(id)kCVPixelBufferPixelFormatTypeKey] stringValue] UTF8String]);
		VUserLog("kCVPixelFormatName                : %s", [[[outputPixelAttribs objectForKey:(id)kCVPixelFormatName] stringValue] UTF8String]);
		VUserLog("kCVPixelFormatConstant            : %s", [[[outputPixelAttribs objectForKey:(id)kCVPixelFormatConstant] stringValue] UTF8String]);
		VUserLog("kCVPixelFormatCodecType           : %s", [[[outputPixelAttribs objectForKey:(id)kCVPixelFormatCodecType] stringValue] UTF8String]);
		VUserLog("kCVPixelFormatFourCC              : %s", [[[outputPixelAttribs objectForKey:(id)kCVPixelFormatFourCC] stringValue] UTF8String]);
		VUserLog("kCVPixelFormatBitsPerBlock        : %ld",[[outputPixelAttribs objectForKey:(id)kCVPixelFormatBitsPerBlock] integerValue]);
		VUserLog("kCVPixelFormatOpenGLFormat        : %s", VuoGl_stringForConstant([[outputPixelAttribs objectForKey:(id)kCVPixelFormatOpenGLFormat] integerValue]));
		VUserLog("kCVPixelFormatOpenGLInternalFormat: %s", VuoGl_stringForConstant([[outputPixelAttribs objectForKey:(id)kCVPixelFormatOpenGLInternalFormat] integerValue]));
		VUserLog("kCVPixelFormatOpenGLType          : %s", VuoGl_stringForConstant([[outputPixelAttribs objectForKey:(id)kCVPixelFormatOpenGLType] integerValue]));
		VUserLog("kCVPixelFormatOpenGLCompatibility : %ld",[[outputPixelAttribs objectForKey:(id)kCVPixelFormatOpenGLCompatibility] integerValue]);

		VUserLog("--------------------------");
	}

	if(callback == NULL) return;

	CVBufferRetain(imageBuffer);

	VuoImage image = NULL;

	@synchronized (self)
	{
		BOOL isFlipped = CVImageBufferIsFlipped(imageBuffer);

		if (firstRun && VuoIsDebugEnabled())
		{
			CGColorSpaceRef colorSpace = CVImageBufferGetColorSpace(imageBuffer);
			CGColorSpaceModel model = CGColorSpaceGetModel(colorSpace);

			if( model == kCGColorSpaceModelUnknown )
				VUserLog("ColorSpace: kCGColorSpaceModelUnknown");
			else
			if( model == kCGColorSpaceModelMonochrome )
				VUserLog("ColorSpace: kCGColorSpaceModelMonochrome");
			else
			if( model == kCGColorSpaceModelRGB )
				VUserLog("ColorSpace: kCGColorSpaceModelRGB");
			else
			if( model == kCGColorSpaceModelCMYK )
				VUserLog("ColorSpace: kCGColorSpaceModelCMYK");
			else
			if( model == kCGColorSpaceModelLab )
				VUserLog("ColorSpace: kCGColorSpaceModelLab");
			else
			if( model == kCGColorSpaceModelDeviceN )
				VUserLog("ColorSpace: kCGColorSpaceModelDeviceN");
			else
			if( model == kCGColorSpaceModelIndexed )
				VUserLog("ColorSpace: kCGColorSpaceModelIndexed");
			else
			if( model == kCGColorSpaceModelPattern )
				VUserLog("ColorSpace: kCGColorSpaceModelPattern");
			else
				VUserLog("ColorSpace: Unknown!!");


			CGSize es = CVImageBufferGetEncodedSize(imageBuffer);
			VUserLog("encodedSize: %gx%g",es.width,es.height);

			CGSize ds = CVImageBufferGetDisplaySize(imageBuffer);
			VUserLog("displaySize: %gx%g",ds.width,ds.height);

			CGRect cr = CGRectIntegral(CVImageBufferGetCleanRect(imageBuffer));
			VUserLog("cleanRect:   %gx%g @ %g,%g",cr.size.width,cr.size.height,cr.origin.x,cr.origin.y);

			VUserLog("Frame [%s] %dx%d flipped=%d bpr=%ld ds=%ld (planar=%d planes=%ld %ldx%ld bpr=%ld)",
				FourCC2Str(CVPixelBufferGetPixelFormatType(imageBuffer)),
				texWidth, texHeight,
				isFlipped,
				CVPixelBufferGetBytesPerRow(imageBuffer),
				CVPixelBufferGetDataSize(imageBuffer),
				CVPixelBufferIsPlanar(imageBuffer),
				CVPixelBufferGetPlaneCount(imageBuffer),
				CVPixelBufferGetWidthOfPlane(imageBuffer, 0), CVPixelBufferGetHeightOfPlane(imageBuffer, 0),
				CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, 0)
			);

			VUserLog("bytes per block: %li", CVPixelBufferGetBytesPerRow(imageBuffer) / texWidth);

			size_t extraColumnsOnLeft;
			size_t extraColumnsOnRight;
			size_t extraRowsOnTop;
			size_t extraRowsOnBottom;

			CVPixelBufferGetExtendedPixels(imageBuffer, &extraColumnsOnLeft, &extraColumnsOnRight, &extraRowsOnTop, &extraRowsOnBottom );

			VUserLog("extraColumnsOnLeft: %lu extraColumnsOnRight: %lu extraRowsOnTop: %lu extraRowsOnBottom: %lu",
				extraColumnsOnLeft,
				extraColumnsOnRight,
				extraRowsOnTop,
				extraRowsOnBottom);

			CIImage *ciImage = [CIImage imageWithCVImageBuffer:imageBuffer];
			CGRect rect = [ciImage extent];

			VUserLog("CIImage Size: %f, %f", rect.size.width, rect.size.height);

			NSBitmapImageRep *newRep = [[NSBitmapImageRep alloc] initWithCIImage:ciImage];

			NSSize size;
			size.width = rect.size.width;
			size.width = rect.size.height;
			[newRep setSize:size];

			NSData *pngData = [newRep representationUsingType:NSPNGFileType properties:nil];
			[newRep autorelease];

			[pngData writeToFile: [@"/tmp/Vuo-QT-Debug-Frame.png" stringByExpandingTildeInPath] atomically: YES];

			firstRun = false;
		}

		CVOpenGLTextureRef texture;
		CVReturn ret = CVOpenGLTextureCacheCreateTextureFromImage(NULL, textureCache, imageBuffer, NULL, &texture);

		if (ret != kCVReturnSuccess)
		{
			VUserLog("Error: %d", ret);
		}
		else
		{
			VuoImage rectImage = VuoImage_makeClientOwnedGlTextureRectangle(
						CVOpenGLTextureGetName(texture),
						GL_RGB,
						CVPixelBufferGetWidth(imageBuffer),
						CVPixelBufferGetHeight(imageBuffer),
						VuoAvPlayerObject_freeCallback, NULL);
			VuoRetain(rectImage);
			image = VuoImage_makeCopy(rectImage, CVOpenGLTextureIsFlipped(texture));
			CVOpenGLTextureRelease(texture);
			VuoRelease(rectImage);
			CVOpenGLTextureCacheFlush(textureCache, 0);
		}
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
 * Returns a C string describing the specified pixel format.
 */
+ (char*) formatTypeString:(int)key
{
	if(key == kCVPixelFormatType_1Monochrome)
		return "kCVPixelFormatType_1Monochrome";
	else if(key == kCVPixelFormatType_2Indexed)
		return "kCVPixelFormatType_2Indexed";
	else if(key == kCVPixelFormatType_4Indexed)
		return "kCVPixelFormatType_4Indexed";
	else if(key == kCVPixelFormatType_8Indexed)
		return "kCVPixelFormatType_8Indexed";
	else if(key == kCVPixelFormatType_1IndexedGray_WhiteIsZero)
		return "kCVPixelFormatType_1IndexedGray_WhiteIsZero";
	else if(key == kCVPixelFormatType_2IndexedGray_WhiteIsZero)
		return "kCVPixelFormatType_2IndexedGray_WhiteIsZero";
	else if(key == kCVPixelFormatType_4IndexedGray_WhiteIsZero)
		return "kCVPixelFormatType_4IndexedGray_WhiteIsZero";
	else if(key == kCVPixelFormatType_8IndexedGray_WhiteIsZero)
		return "kCVPixelFormatType_8IndexedGray_WhiteIsZero";
	else if(key == kCVPixelFormatType_16BE555)
		return "kCVPixelFormatType_16BE555";
	else if(key == kCVPixelFormatType_16LE555)
		return "kCVPixelFormatType_16LE555";
	else if(key == kCVPixelFormatType_16LE5551)
		return "kCVPixelFormatType_16LE5551";
	else if(key == kCVPixelFormatType_16BE565)
		return "kCVPixelFormatType_16BE565";
	else if(key == kCVPixelFormatType_16LE565)
		return "kCVPixelFormatType_16LE565";
	else if(key == kCVPixelFormatType_24RGB)
		return "kCVPixelFormatType_24RGB";
	else if(key == kCVPixelFormatType_24BGR)
		return "kCVPixelFormatType_24BGR";
	else if(key == kCVPixelFormatType_32ARGB)
		return "kCVPixelFormatType_32ARGB";
	else if(key == kCVPixelFormatType_32BGRA)
		return "kCVPixelFormatType_32BGRA";
	else if(key == kCVPixelFormatType_32ABGR)
		return "kCVPixelFormatType_32ABGR";
	else if(key == kCVPixelFormatType_32RGBA)
		return "kCVPixelFormatType_32RGBA";
	else if(key == kCVPixelFormatType_64ARGB)
		return "kCVPixelFormatType_64ARGB";
	else if(key == kCVPixelFormatType_48RGB)
		return "kCVPixelFormatType_48RGB";
	else if(key == kCVPixelFormatType_32AlphaGray)
		return "kCVPixelFormatType_32AlphaGray";
	else if(key == kCVPixelFormatType_16Gray)
		return "kCVPixelFormatType_16Gray";
	else if(key == kCVPixelFormatType_30RGB)
		return "kCVPixelFormatType_30RGB";
	else if(key == kCVPixelFormatType_422YpCbCr8)
		return "kCVPixelFormatType_422YpCbCr8";
	else if(key == kCVPixelFormatType_4444YpCbCrA8)
		return "kCVPixelFormatType_4444YpCbCrA8";
	else if(key == kCVPixelFormatType_4444YpCbCrA8R)
		return "kCVPixelFormatType_4444YpCbCrA8R";
	else if(key == kCVPixelFormatType_4444AYpCbCr8)
		return "kCVPixelFormatType_4444AYpCbCr8";
	else if(key == kCVPixelFormatType_4444AYpCbCr16)
		return "kCVPixelFormatType_4444AYpCbCr16";
	else if(key == kCVPixelFormatType_444YpCbCr8)
		return "kCVPixelFormatType_444YpCbCr8";
	else if(key == kCVPixelFormatType_422YpCbCr16)
		return "kCVPixelFormatType_422YpCbCr16";
	else if(key == kCVPixelFormatType_422YpCbCr10)
		return "kCVPixelFormatType_422YpCbCr10";
	else if(key == kCVPixelFormatType_444YpCbCr10)
		return "kCVPixelFormatType_444YpCbCr10";
	else if(key == kCVPixelFormatType_420YpCbCr8Planar)
		return "kCVPixelFormatType_420YpCbCr8Planar";
	else if(key == kCVPixelFormatType_420YpCbCr8PlanarFullRange)
		return "kCVPixelFormatType_420YpCbCr8PlanarFullRange";
	else if(key == kCVPixelFormatType_422YpCbCr_4A_8BiPlanar)
		return "kCVPixelFormatType_422YpCbCr_4A_8BiPlanar";
	else if(key == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
		return "kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange";
	else if(key == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange)
		return "kCVPixelFormatType_420YpCbCr8BiPlanarFullRange";
	else if(key == kCVPixelFormatType_422YpCbCr8_yuvs)
		return "kCVPixelFormatType_422YpCbCr8_yuvs";
	else if(key == kCVPixelFormatType_422YpCbCr8FullRange)
		return "kCVPixelFormatType_422YpCbCr8FullRange";
/*
 * 10.8+ only
	else if(key == kCVPixelFormatType_OneComponent8)
		return "kCVPixelFormatType_OneComponent8";
	else if(key == kCVPixelFormatType_TwoComponent8)
		return "kCVPixelFormatType_TwoComponent8";
*/
/*
 * 10.9+ only
	else if(key == kCVPixelFormatType_OneComponent16Half)
		return "kCVPixelFormatType_OneComponent16Half";
	else if(key == kCVPixelFormatType_OneComponent32Float)
		return "kCVPixelFormatType_OneComponent32Float";
	else if(key == kCVPixelFormatType_TwoComponent16Half)
		return "kCVPixelFormatType_TwoComponent16Half";
	else if(key == kCVPixelFormatType_TwoComponent32Float)
		return "kCVPixelFormatType_TwoComponent32Float";
	else if(key == kCVPixelFormatType_64RGBAHalf)
		return "kCVPixelFormatType_64RGBAHalf";
	else if(key == kCVPixelFormatType_128RGBAFloat)
		return "kCVPixelFormatType_128RGBAFloat";
*/
	else
		return "key not found";
}

@end
