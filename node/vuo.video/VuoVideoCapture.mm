/**
 * @file
 * VuoVideoCapture implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "module.h"
#include <time.h>
#include "VuoApp.h"
#include "VuoVideoCapture.h"
#include "VuoTriggerSet.hh"
#include "VuoOsStatus.h"
#include "../vuo.hid/VuoUsbVendor.h"

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#include <IOKit/IOKitLib.h>
#include <CoreMediaIO/CMIOHardware.h>
#include <AVFoundation/AVFoundation.h>
#import <OpenGL/gl.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoVideoCapture",
					  "dependencies" : [
						"VuoImage",
						"VuoOsStatus",
						"VuoUsbVendor",
						"VuoVideoInputDevice",
						"VuoList_VuoVideoInputDevice",
						"AVFoundation.framework",
						"CoreMedia.framework",
						"CoreMediaIO.framework",
						"CoreVideo.framework",
						"IOKit.framework",
						"AppKit.framework",
					  ]
				 });
#endif

extern "C"
{

/**
 * Enable support for video capture from tethered iOS devices.
 */
static void VuoVideoCapture_enableTethering()
{
	static dispatch_once_t enableOnce = 0;
	dispatch_once(&enableOnce, ^{
		VuoApp_executeOnMainThread(^{
			CMIOObjectPropertyAddress property = { kCMIOHardwarePropertyAllowScreenCaptureDevices, kCMIOObjectPropertyScopeGlobal, kCMIOObjectPropertyElementMaster };
			double t0 = VuoLogGetTime();
			bool hasProperty = CMIOObjectHasProperty(kCMIOObjectSystemObject, &property);
			double t1 = VuoLogGetTime();
			if (t1 - t0 > 1)
				VUserLog("Warning: Apple CoreMediaIO took %.1f seconds to initialize its 3rd-party plugins.", t1 - t0);

			if (hasProperty)
			{
				UInt32 yes = 1;
				// This call sometimes blocks the main thread for about 5 seconds (!).
				OSStatus ret = CMIOObjectSetPropertyData(kCMIOObjectSystemObject, &property, 0, NULL, sizeof(yes), &yes);
				if (ret != kCMIOHardwareNoError)
				{
					char *errorText = VuoOsStatus_getText(ret);
					VUserLog("Warning: Couldn't enable tethered iOS device support: %s", errorText);
					free(errorText);
				}
			}
		});
	});
}

}

@class VuoVideoCaptureDelegate;

/**
 * Data for a video capture instance.
 */
typedef struct
{
	CVOpenGLTextureCacheRef textureCache;              ///< A quick way to convert a CGPixelBuffer to an OpenGL texture.
	dispatch_queue_t queue;                            ///< Serializes access to glContext.
	VuoVideoInputDevice vdevice;                       ///< Information for selecting a camera.
	AVCaptureDevice *device;                           ///< The camera we're capturing from.
	AVCaptureDeviceInput *input;                       ///< Connects the camera to the session.
	AVCaptureSession *session;                         ///< Manages capturing video.
	VuoVideoCaptureDelegate *delegate;                 ///< Internal class invoked when a frame is captured.
	void (*receivedFrameTrigger)(VuoVideoFrame image); ///< The node callback to invoke when a frame is captured.
	int preferredWidth;                                ///< The video width requested by the composition.
	int preferredHeight;                               ///< The video height requested by the composition.
	bool preferredListening;                           ///< Whether the composition has requested that we start listening for frames.
	AVCaptureDeviceFormat *preferredFormat;            ///< The best-matching format selected by @ref VuoVideoCapture_setSize.
	int attemptedSetSizes;                             ///< How many times AV Foundation has changed the size to something other than what we requested.
	double firstFrameTime;                             ///< The presentation timestamp of the first frame we received.
} VuoVideoCaptureInternal;

static dispatch_queue_t VuoVideoCapture_pendingDevicesQueue;                ///< Serializes access to `VuoVideoCapture_pendingDevices`.
static std::set<VuoVideoCaptureInternal *> VuoVideoCapture_pendingDevices;  ///< Devices that have been requested but haven't yet been successfully opened.

void VuoVideoCapture_openDevice(VuoVideoCaptureInternal *vci);

/**
 * Attempts to open each device that has been requested but hasn't yet been successfully opened.
 *
 * @threadQueue{VuoVideoCapture_pendingDevicesQueue}
 */
void VuoVideoCapture_checkPendingDevices(void *)
{
	for (auto dev = VuoVideoCapture_pendingDevices.begin(); dev != VuoVideoCapture_pendingDevices.end();)
	{
		VuoVideoCapture_openDevice(*dev);
		if ((*dev)->device)
			VuoVideoCapture_pendingDevices.erase(*dev++);
		else
			++dev;
	}
}

/**
 * A singleton class that listens for changes in available video input devices.
 */
@interface VuoVideoCaptureDeviceListener : NSObject
{
	VuoTriggerSet<VuoList_VuoVideoInputDevice> callbacks;                           ///< A TriggerSet of VuoList_VuoVideoInputDevice output triggers.
}

- (id) init;                                                                        ///< Init and register for NSNotification events.
- (void) devicesDidChange:(NSNotification*)notif;                                   ///< Called when a change is detected.
- (void) addCallback:(void(*)(VuoList_VuoVideoInputDevice)) outputTrigger;          ///< Add a callback to be triggered when the input list changes.
- (void) removeCallback:(void(*)(VuoList_VuoVideoInputDevice)) outputTrigger;       ///< Remove a callback from the trigger set.
@end

@implementation VuoVideoCaptureDeviceListener

- (id) init
{
	if (self = [super init])
	{
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(devicesDidChange:) name:AVCaptureDeviceWasConnectedNotification object:nil];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(devicesDidChange:) name:AVCaptureDeviceWasDisconnectedNotification object:nil];
	}

	return self;
}

- (void)dealloc
{
	[NSNotificationCenter.defaultCenter removeObserver:self];
	[super dealloc];
}

- (void) addCallback:(void(*)(VuoList_VuoVideoInputDevice)) outputTrigger
{
	callbacks.addTrigger(outputTrigger);
	outputTrigger(VuoVideoCapture_getInputDevices());
}

- (void) removeCallback:(void(*)(VuoList_VuoVideoInputDevice)) outputTrigger
{
	callbacks.removeTrigger(outputTrigger);
}

- (void) devicesDidChange:(NSNotification*)notif
{
	callbacks.fire(VuoVideoCapture_getInputDevices());
	dispatch_async_f(VuoVideoCapture_pendingDevicesQueue, nullptr, VuoVideoCapture_checkPendingDevices);
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(AVCaptureDevice *)object change:(NSDictionary *)change context:(void *)p
{
	VuoVideoCaptureInternal *vci = (VuoVideoCaptureInternal *)p;
	if ([keyPath isEqualToString:@"connected"]
	 && !object.connected)
	{
		VUserLog("Warning: %s was disconnected.  I'll keep looking for it.", VuoVideoInputDevice_getSummary(vci->vdevice));

		[vci->device removeObserver:self forKeyPath:@"connected" context:vci];

		[vci->session removeInput:vci->input];
		vci->input = nil;
		dispatch_async(VuoVideoCapture_pendingDevicesQueue, ^{
			VuoVideoCapture_pendingDevices.insert(vci);
		});
	}
}
@end

static VuoVideoCaptureDeviceListener *deviceListener;  ///< A static reference to the VuoVideoCaptureDeviceListener singleton.

/**
 * Starts listening for device-change notifications.
 */
void VuoVideoCapture_init()
{
	VuoVideoCapture_enableTethering();

	static dispatch_once_t init = 0;
	dispatch_once(&init, ^{
		deviceListener = [[VuoVideoCaptureDeviceListener alloc] init];
		VuoVideoCapture_pendingDevicesQueue = dispatch_queue_create("org.vuo.video.pendingDevicesQueue", NULL);
	});
}

/**
 * Removes an output trigger from the VuoVideoCaptureDeviceListener TriggerSet.
 */
void VuoVideoCapture_removeOnDevicesChangedCallback( VuoOutputTrigger(devicesDidChange, VuoList_VuoVideoInputDevice) )
{
	if(deviceListener != nil)
		[deviceListener removeCallback:devicesDidChange];
}

/**
 * Adds an output trigger to the VuoVideoCaptureDeviceListener TriggerSet to be fired when available input devices changes.
 */
void VuoVideoCapture_addOnDevicesChangedCallback( VuoOutputTrigger(devicesDidChange, VuoList_VuoVideoInputDevice) )
{
	VuoVideoCapture_init();
	[deviceListener addCallback:devicesDidChange];
}

/**
 * Given a `uniqueID` string (e.g., `0x1a11000005ac8510`, from `-[QTCaptureDevice uniqueID]`),
 * searches the IORegistry to find a matching FireWire or USB device,
 * and returns the Vendor Name string.
 *
 * The returned string has a retain count +1, so the caller should release it.
 */
NSString *VuoVideoCapture_getVendorNameForUniqueID(NSString *uniqueID)
{
	CFMutableDictionaryRef match_dictionary = IOServiceMatching("IOFireWireDevice");
	io_iterator_t entry_iterator;
	if (IOServiceGetMatchingServices(kIOMasterPortDefault, match_dictionary, &entry_iterator) == kIOReturnSuccess)
	{
		io_registry_entry_t serviceObject;
		while ((serviceObject = IOIteratorNext(entry_iterator)))
		{
			CFMutableDictionaryRef serviceDictionary;
			if (IORegistryEntryCreateCFProperties(serviceObject, &serviceDictionary, kCFAllocatorDefault, kNilOptions) != kIOReturnSuccess)
			{
				IOObjectRelease(serviceObject);
				continue;
			}

			NSString *guidAsHexString = [NSString stringWithFormat:@"%llx",[(NSNumber *)CFDictionaryGetValue(serviceDictionary, @"GUID") longLongValue]];
			if ([uniqueID rangeOfString:guidAsHexString].location != NSNotFound)
			{
				NSString *vendorName = [(NSString *)CFDictionaryGetValue(serviceDictionary, @"FireWire Vendor Name") retain];
				CFRelease(serviceDictionary);
				IOObjectRelease(serviceObject);
				IOObjectRelease(entry_iterator);
				return vendorName;
			}

			CFRelease(serviceDictionary);
			IOObjectRelease(serviceObject);
		}
		IOObjectRelease(entry_iterator);
	}

	match_dictionary = IOServiceMatching("IOUSBDevice");
	if (IOServiceGetMatchingServices(kIOMasterPortDefault, match_dictionary, &entry_iterator) == kIOReturnSuccess)
	{
		io_registry_entry_t serviceObject;
		while ((serviceObject = IOIteratorNext(entry_iterator)))
		{
			CFMutableDictionaryRef serviceDictionary;
			if (IORegistryEntryCreateCFProperties(serviceObject, &serviceDictionary, kCFAllocatorDefault, kNilOptions) != kIOReturnSuccess)
			{
				IOObjectRelease(serviceObject);
				continue;
			}

			NSString *guidAsHexString = [NSString stringWithFormat:@"%llx",[(NSNumber *)CFDictionaryGetValue(serviceDictionary, @"locationID") longLongValue]];
			if ([uniqueID rangeOfString:guidAsHexString].location != NSNotFound)
			{
				NSString *vendorName = [(NSString *)CFDictionaryGetValue(serviceDictionary, @"USB Vendor Name") retain];
				if (!vendorName.length)
				{
					char *vendor = VuoUsbVendor_getText(((NSNumber *)CFDictionaryGetValue(serviceDictionary, @"idVendor")).intValue);
					vendorName = [[NSString stringWithUTF8String:vendor] retain];
					free(vendor);
				}

				CFRelease(serviceDictionary);
				IOObjectRelease(serviceObject);
				IOObjectRelease(entry_iterator);
				return vendorName;
			}

			CFRelease(serviceDictionary);
			IOObjectRelease(serviceObject);
		}
		IOObjectRelease(entry_iterator);
	}

	return @"";
}

/**
 * Returns a display name for the specified device.
 */
VuoText VuoVideoCapture_getDeviceName(AVCaptureDevice *dev)
{
	NSString *deviceName = dev.localizedName;

	NSString *vendorName = VuoVideoCapture_getVendorNameForUniqueID([dev uniqueID]);

	NSString *modelWithoutVendor = dev.modelID;
	NSUInteger tail = [modelWithoutVendor rangeOfString:@" Camera VendorID_"].location;
	if (tail != NSNotFound)
		modelWithoutVendor = [modelWithoutVendor substringToIndex:tail];
	if (modelWithoutVendor.length
	 && [dev.manufacturer rangeOfString:modelWithoutVendor].location == NSNotFound)
		deviceName = [NSString stringWithFormat:@"%@ %@", modelWithoutVendor, deviceName];

	if (![dev.manufacturer isEqualToString:vendorName]
	 && ![dev.manufacturer isEqualToString:@"Unknown"])
		deviceName = [NSString stringWithFormat:@"%@ %@", dev.manufacturer, deviceName];

	if ([vendorName length])
		deviceName = [NSString stringWithFormat:@"%@ %@", vendorName, deviceName];

	// manufacturer=0x00000001 LLC.    IOReg=                 modelID=                                                 name=VirtualEyez HD             result=[0x00000001 LLC. VirtualEyez HD]
	// manufacturer=Allocinit.com      IOReg=                 modelID=CamTwist                                         name=CamTwist                   result=[Allocinit.com CamTwist CamTwist]
	// manufacturer=Apple Inc.         IOReg=                 modelID=Apple Camera VendorID_0x106B ProductID_0x1570    name=FaceTime HD Camera         result=[Apple Inc. FaceTime HD Camera]
	// manufacturer=Apple Inc.         IOReg=                 modelID=iOS Device                                       name=smokris iPhone             result=[Apple Inc. iOS Device smokris iPhone]
	// manufacturer=Canon              IOReg=Canon            modelID=                                                 name=VIXIA HV30                 result=[Canon VIXIA HV30]
	// manufacturer=Kinoni             IOReg=                 modelID=EpocCamDevice:0:0                                name=EpocCam                    result=[Kinoni EpocCamDevice:0:0 EpocCam]
	// manufacturer=SnapVendor         IOReg=                 modelID=SnapCamera                                       name=Snap Camera                result=[SnapVendor SnapCamera Snap Camera]
	// manufacturer=Telestream         IOReg=                 modelID=WCVC-0.4.0                                       name=Wirecast Virtual Camera    result=[Telestream WCVC-0.4.0 Wirecast Virtual Camera]
	// manufacturer=Unknown            IOReg=Logitech Inc.    modelID=UVC Camera VendorID_1133 ProductID_2053          name=USB Camera #2              result=[Logitech Inc. UVC USB Camera #2]
	// manufacturer=Unknown            IOReg=Logitech Inc.    modelID=UVC Camera VendorID_1133 ProductID_2081          name=USB Camera                 result=[Logitech Inc. UVC USB Camera]
	// manufacturer=Unknown            IOReg=Logitech Inc.    modelID=UVC Camera VendorID_1133 ProductID_2093          name=HD Pro Webcam C920         result=[Logitech Inc. UVC HD Pro Webcam C920]
	// manufacturer=Visicom Inc.       IOReg=                 modelID=                                                 name=ManyCam Virtual Webcam     result=[Visicom Inc. ManyCam Virtual Webcam]
//	NSLog(@"manufacturer=%@    IOReg=%@    modelID=%@    name=%@    result=[%@]", dev.manufacturer, vendorName, dev.modelID, dev.localizedName, deviceName);

	[vendorName release];
	return VuoText_make([deviceName UTF8String]);
}

/**
 * Returns a list of all available video capture devices.
 */
VuoList_VuoVideoInputDevice VuoVideoCapture_getInputDevices(void)
{
	VuoVideoCapture_init();

	// Use +devices instead of +devicesWithMediaType:AVMediaTypeVideo,
	// since the latter doesn't include tethered iOS devices.
	NSArray *inputDevices = [AVCaptureDevice devices];

	VuoList_VuoVideoInputDevice devices = VuoListCreate_VuoVideoInputDevice();

	for (AVCaptureDevice *dev in inputDevices)
	{
		if (![dev hasMediaType:AVMediaTypeVideo]
		 && ![dev hasMediaType:AVMediaTypeMuxed])
			continue;

		VuoText uniqueID    = VuoText_make([[dev uniqueID]             UTF8String]);

		VuoListAppendValue_VuoVideoInputDevice(devices, VuoVideoInputDevice_make(uniqueID, VuoVideoCapture_getDeviceName(dev)));
	}

	return devices;
}

void VuoVideoCapture_free(VuoVideoCapture movie);

/**
 * Does nothing.
 */
static void VuoVideoCapture_freeCallback(VuoImage imageToFree)
{
}

/**
 * Callback to process captured frames.
 */
@interface VuoVideoCaptureDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@property VuoVideoCaptureInternal *vci;  ///< Internal data.
@end

@implementation VuoVideoCaptureDelegate
/**
 * Processes a frame captured by the AVCaptureSession.
 *
 * @threadQueue{VuoVideoCaptureInternal::queue}
 */
- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
	CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
	if (!pixelBuffer)
	{
		VUserLog("Warning: AV Foundation gave us a sampleBuffer without a pixelBuffer.");
		return;
	}

	size_t pixelBufferWidth  = CVPixelBufferGetWidth(pixelBuffer);
	size_t pixelBufferHeight = CVPixelBufferGetHeight(pixelBuffer);

	// Work around AV Foundation bug where it sometimes ignores format changes on some cameras (e.g., 640x480@60FPS Logitech C910).
	CMVideoDimensions preferredDim = CMVideoFormatDescriptionGetDimensions(_vci->preferredFormat.formatDescription);
	if ((preferredDim.width  && pixelBufferWidth  != preferredDim.width)
	 || (preferredDim.height && pixelBufferHeight != preferredDim.height))
	{
		const int maxSetSizeAttempts = 10;
		if (_vci->attemptedSetSizes < maxSetSizeAttempts)
		{
			++_vci->attemptedSetSizes;
			VUserLog("Warning: We requested %dx%d, but the camera gave us %zux%zu.  Trying again (attempt %d of %d)…",
					 preferredDim.width, preferredDim.height,
					 pixelBufferWidth, pixelBufferHeight,
					 _vci->attemptedSetSizes, maxSetSizeAttempts);
			VuoVideoCapture_setSize(_vci, _vci->preferredWidth, _vci->preferredHeight);
			return;
		}
		else if (_vci->attemptedSetSizes == maxSetSizeAttempts)
		{
			++_vci->attemptedSetSizes;
			VUserLog("Warning: The camera didn't get the hint after %d attempts, so we'll just let it do what it wants…", maxSetSizeAttempts);
		}
	}

	__block CVOpenGLTextureRef texture;
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		CVOpenGLTextureCacheCreateTextureFromImage(NULL, _vci->textureCache, pixelBuffer, NULL, &texture);
	});

	NSDictionary *pixelAspectRatio = (NSDictionary *)CVBufferGetAttachment(pixelBuffer, CFSTR("CVPixelAspectRatio"), nullptr);
	unsigned int stretchedWidth = 0;
	if (pixelAspectRatio)
		stretchedWidth = pixelBufferWidth * ((NSNumber *)pixelAspectRatio[@"HorizontalSpacing"]).intValue / ((NSNumber *)pixelAspectRatio[@"VerticalSpacing"]).intValue;

	VuoImage rectImage = VuoImage_makeClientOwnedGlTextureRectangle(
		CVOpenGLTextureGetName(texture),
		GL_RGB,
		pixelBufferWidth,
		pixelBufferHeight,
		VuoVideoCapture_freeCallback, NULL);
	VuoRetain(rectImage);
	VuoImage image = VuoImage_makeCopy(rectImage, CVOpenGLTextureIsFlipped(texture), stretchedWidth, 0, false);
	if (_vci->firstFrameTime == -INFINITY)
		_vci->firstFrameTime = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sampleBuffer));
	if (_vci->receivedFrameTrigger)
		_vci->receivedFrameTrigger(VuoVideoFrame_make(image,
													  CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sampleBuffer)) - _vci->firstFrameTime,
													  CMTimeGetSeconds(CMSampleBufferGetDuration(sampleBuffer))));

	CVOpenGLTextureRelease(texture);
	VuoRelease(rectImage);

	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		CVOpenGLTextureCacheFlush(_vci->textureCache, 0);
	});
}
@end

/**
 * Tries to find and open an AVCaptureDevice matching the VuoVideoInputDevice spec.
 */
void VuoVideoCapture_openDevice(VuoVideoCaptureInternal *vci)
{
	if (VuoText_isEmpty(vci->vdevice.name) && VuoText_isEmpty(vci->vdevice.id))
		vci->device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
	else
	{
		vci->device = [AVCaptureDevice deviceWithUniqueID:[NSString stringWithUTF8String:vci->vdevice.id]];

		// If that fails, try to match the device name
		if (vci->vdevice.matchType == VuoVideoInputDevice_MatchIdThenName
			&& !VuoText_isEmpty(vci->vdevice.name)
			&& !vci->device)
		{
			for (AVCaptureDevice *dev in AVCaptureDevice.devices)
			{
				if (![dev hasMediaType:AVMediaTypeVideo]
					&& ![dev hasMediaType:AVMediaTypeMuxed])
					continue;

				if (strstr(VuoVideoCapture_getDeviceName(dev), vci->vdevice.name))
				{
					vci->device = dev;
					break;
				}
			}
		}
	}

	if (vci->device)
	{
		[vci->device addObserver:deviceListener forKeyPath:@"connected" options:0 context:vci];

		VuoVideoCapture_setSize(vci, vci->preferredWidth, vci->preferredHeight);
		if (vci->preferredListening)
			VuoVideoCapture_startListening(vci);

		NSError *e;
		vci->input = [AVCaptureDeviceInput deviceInputWithDevice:vci->device error:&e];
		if (vci->input)
		{
			if ([vci->session canAddInput:vci->input])
				[vci->session addInput:vci->input];
			else
				VUserLog("Error: -[AVCaptureSession canAddInput:] said no.");
		}
		else
			VUserLog("Error: Couldn't create AVCaptureDeviceInput: %s", e.localizedDescription.UTF8String);

		vci->firstFrameTime = -INFINITY;
	}
}

/**
 * Creates a new VuoVideoCapture instance for receiving video frames from `inputDevice`.
 */
VuoVideoCapture VuoVideoCapture_make(VuoVideoInputDevice inputDevice, VuoOutputTrigger(receivedFrame, VuoVideoFrame))
{
	VuoVideoCapture_init();

	if ([AVCaptureDevice respondsToSelector:@selector(authorizationStatusForMediaType:)])
	{
		long status = (long)[AVCaptureDevice performSelector:@selector(authorizationStatusForMediaType:) withObject:@"vide"];
		if (status == 0 /* AVAuthorizationStatusNotDetermined */)
			VUserLog("Warning: Video input may be unavailable due to system restrictions.  Check System Preferences > Security & Privacy > Privacy > Camera.");
		else if (status == 1 /* AVAuthorizationStatusRestricted */
			  || status == 2 /* AVAuthorizationStatusDenied */)
			VUserLog("Error: Video input is unavailable due to system restrictions.  Check System Preferences > Security & Privacy > Privacy > Camera.");
	}

	VuoVideoCaptureInternal *vci = (VuoVideoCaptureInternal *)calloc(1, sizeof(VuoVideoCaptureInternal));
	VuoRegister(vci, VuoVideoCapture_free);

	vci->receivedFrameTrigger = receivedFrame;

	vci->queue = dispatch_queue_create("org.vuo.VuoVideoCapture", NULL);

	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
		VUOLOG_PROFILE_END(mainQueue);
		vci->delegate = [VuoVideoCaptureDelegate new];
		vci->delegate.vci = vci;

		vci->session = [AVCaptureSession new];
		vci->session.sessionPreset = AVCaptureSessionPresetPhoto;

		vci->vdevice = inputDevice;
		VuoVideoInputDevice_retain(vci->vdevice);

		{
			VuoVideoCapture_openDevice(vci);
			if (!vci->device)
			{
				VUserLog("Warning: %s isn't currently available.  I'll keep trying.", VuoVideoInputDevice_getSummary(inputDevice));
				dispatch_async(VuoVideoCapture_pendingDevicesQueue, ^{
					VuoVideoCapture_pendingDevices.insert(vci);
				});
			}
		}

		{
			AVCaptureVideoDataOutput *output = [AVCaptureVideoDataOutput new];
			[output setAlwaysDiscardsLateVideoFrames:YES];
			[output setSampleBufferDelegate:vci->delegate queue:vci->queue];
			[vci->session addOutput:output];
			[output release];
		}
	});

	{
		CGLPixelFormatObj pf = (CGLPixelFormatObj)VuoGlContext_makePlatformPixelFormat(false, false, -1);
		__block CVReturn ret;
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			ret = CVOpenGLTextureCacheCreate(NULL, NULL, cgl_ctx, pf, NULL, &vci->textureCache);
		});
		CGLReleasePixelFormat(pf);

		if (ret != kCVReturnSuccess)
		{
			VUserLog("Error: Couldn't create texture cache: %d", ret);
			VuoVideoCapture_free(vci);
			return NULL;
		}
	}

	return (VuoVideoCapture)vci;
}

/**
 * Specifies the desired image size.
 */
void VuoVideoCapture_setSize(VuoVideoCapture p, VuoInteger width, VuoInteger height)
{
	VuoVideoCaptureInternal *vci = (VuoVideoCaptureInternal *)p;
	if (!vci)
		return;

	vci->preferredWidth  = width;
	vci->preferredHeight = height;

	if (!vci->device)
		return;

	AVCaptureDeviceFormat *bestFormat = nil;
	AVFrameRateRange *bestFrameRateRange = nil;
	if (width == 0 && height == 0)
	{
		// By default, pick the format with the highest framerate.
		for (AVCaptureDeviceFormat *format in vci->device.formats)
			for (AVFrameRateRange *range in format.videoSupportedFrameRateRanges)
				if (range.maxFrameRate > bestFrameRateRange.maxFrameRate)
				{
					bestFormat         = format;
					bestFrameRateRange = range;
				}

		// Then, if there are multiple resolutions with that framerate, pick the highest resolution.
		for (AVCaptureDeviceFormat *format in vci->device.formats)
		{
			CMVideoDimensions dim = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
			for (AVFrameRateRange *range in format.videoSupportedFrameRateRanges)
			{
				CMVideoDimensions bestDim = CMVideoFormatDescriptionGetDimensions(bestFormat.formatDescription);
				if (VuoReal_areEqual(range.maxFrameRate, bestFrameRateRange.maxFrameRate)
				 && dim.width  > bestDim.width
				 && dim.height > bestDim.height)
				{
					bestFormat         = format;
					bestFrameRateRange = range;
				}
			}
		}
	}
	else
	{
		// Pick the format with the closest resolution.
		float bestDist = INFINITY;
		for (AVCaptureDeviceFormat *format in vci->device.formats)
		{
			CMVideoDimensions dim = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
			float dist = pow(dim.width - width, 2) + pow(dim.height - height, 2);
			if (dist < bestDist)
			{
				bestDist = dist;
				bestFormat = format;
				bestFrameRateRange = nil;
				for (AVFrameRateRange *range in format.videoSupportedFrameRateRanges)
					if (range.maxFrameRate > bestFrameRateRange.maxFrameRate)
						bestFrameRateRange = range;
			}
		}
	}

	if (bestFormat)
	{
		if ([vci->device lockForConfiguration:NULL] == YES)
		{
			vci->device.activeFormat = bestFormat;

			[vci->preferredFormat release];
			vci->preferredFormat = [bestFormat retain];
			if (bestFrameRateRange)
			{
				vci->device.activeVideoMinFrameDuration = bestFrameRateRange.minFrameDuration;
				vci->device.activeVideoMaxFrameDuration = bestFrameRateRange.minFrameDuration;
			}

			[vci->device unlockForConfiguration];
		}
		else
			VUserLog("Warning: Couldn't lock the video input device for configuration; ignoring the specified resolution.");
	}
	else
	{
		[vci->preferredFormat release];
		vci->preferredFormat = nil;
		VUserLog("Warning: This device didn't report any supported formats.");
	}
}

/**
 * Begins receiving frames.
 */
void VuoVideoCapture_startListening(VuoVideoCapture p)
{
	VuoVideoCaptureInternal *vci = (VuoVideoCaptureInternal *)p;
	if (!vci)
		return;

	vci->preferredListening = true;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
		VUOLOG_PROFILE_END(mainQueue);
		[vci->session startRunning];
	});
}

/**
 * Stops receiving frames (starts ignoring/dropping them).
 */
void VuoVideoCapture_stopListening(VuoVideoCapture p)
{
	VuoVideoCaptureInternal *vci = (VuoVideoCaptureInternal *)p;
	if (!vci)
		return;

	vci->preferredListening = false;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
		VUOLOG_PROFILE_END(mainQueue);
		[vci->session stopRunning];
	});
}

/**
 * Sets the image callback.
 */
void VuoVideoCapture_setCallback(VuoVideoCapture p, VuoOutputTrigger(receivedFrame, VuoVideoFrame))
{
	VuoVideoCaptureInternal *vci = (VuoVideoCaptureInternal *)p;
	if (!vci)
		return;

	vci->receivedFrameTrigger = receivedFrame;
}

/**
 * Releases video capture objects.  Blocks until all captured frames have been fired.
 */
void VuoVideoCapture_free(void *p)
{
	VuoVideoCaptureInternal *vci = (VuoVideoCaptureInternal *)p;

	dispatch_sync(VuoVideoCapture_pendingDevicesQueue, ^{
		VuoVideoCapture_pendingDevices.erase(vci);
	});

	[vci->device removeObserver:deviceListener forKeyPath:@"connected" context:vci];

	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
		VUOLOG_PROFILE_END(mainQueue);
		[vci->session stopRunning];
	});
	dispatch_sync(vci->queue, ^{});

	[vci->session release];
	[vci->delegate release];

	if (vci->textureCache)
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			CVOpenGLTextureCacheRelease(vci->textureCache);
		});

	dispatch_release(vci->queue);

	VuoVideoInputDevice_release(vci->vdevice);
}
