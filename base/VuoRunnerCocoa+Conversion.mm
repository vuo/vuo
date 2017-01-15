/**
 * @file
 * VuoRunnerCocoa+Conversion implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRunnerCocoa+Conversion.hh"

#include <OpenGL/CGLMacro.h>
#include <QuartzCore/CoreImage.h>
#include <QuartzCore/CoreVideo.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json-c/json.h>
#pragma clang diagnostic pop

#include <vector>
#include "VuoStringUtilities.hh"
#include "VuoType.hh"

extern "C" {
#include "VuoHeap.h"
#include "VuoGlContext.h"

#include "../node/node.h"
}

@implementation VuoRunnerCocoa (Conversion)

/**
 * Given the specified @c vuoValue (serialized as JSON), returns a Cocoa object.
 *
 * @see valueForOututPort:
 */
+ (id)cocoaObjectWithVuoValue:(json_object *)vuoValue ofType:(string)type
{
	if (type == "VuoBoolean")
	{
		VuoBoolean v = VuoBoolean_makeFromJson(vuoValue);
		return [NSNumber numberWithBool:v];
	}
	if (type == "VuoList_VuoBoolean")
	{
		VuoList_VuoBoolean l = VuoList_VuoBoolean_makeFromJson(vuoValue);
		unsigned long count = VuoListGetCount_VuoBoolean(l);
		NSMutableArray *a = [[NSMutableArray new] autorelease];
		for (unsigned long i=0; i<count; ++i)
			[a addObject:[NSNumber numberWithBool:VuoListGetValue_VuoBoolean(l, i+1)]];
		return a;
	}

	if (type == "VuoInteger")
	{
		VuoInteger v = VuoInteger_makeFromJson(vuoValue);
		return [NSNumber numberWithLong:v];
	}
	if (type == "VuoList_VuoInteger")
	{
		VuoList_VuoInteger l = VuoList_VuoInteger_makeFromJson(vuoValue);
		unsigned long count = VuoListGetCount_VuoInteger(l);
		NSMutableArray *a = [[NSMutableArray new] autorelease];
		for (unsigned long i=0; i<count; ++i)
			[a addObject:[NSNumber numberWithLong:VuoListGetValue_VuoInteger(l, i+1)]];
		return a;
	}

	if (type == "VuoReal")
	{
		VuoReal v = VuoReal_makeFromJson(vuoValue);
		return [NSNumber numberWithDouble:v];
	}
	if (type == "VuoList_VuoReal")
	{
		VuoList_VuoReal l = VuoList_VuoReal_makeFromJson(vuoValue);
		unsigned long count = VuoListGetCount_VuoReal(l);
		NSMutableArray *a = [[NSMutableArray new] autorelease];
		for (unsigned long i=0; i<count; ++i)
			[a addObject:[NSNumber numberWithDouble:VuoListGetValue_VuoReal(l, i+1)]];
		return a;
	}

	if (type == "VuoText")
	{
		VuoText v = VuoText_makeFromJson(vuoValue);
		VuoRetain(v);
		NSString *s = [NSString stringWithUTF8String:v];
		VuoRelease(v);
		return s;
	}
	if (type == "VuoList_VuoText")
	{
		VuoList_VuoText l = VuoList_VuoText_makeFromJson(vuoValue);
		unsigned long count = VuoListGetCount_VuoText(l);
		NSMutableArray *a = [[NSMutableArray new] autorelease];
		for (unsigned long i=0; i<count; ++i)
			[a addObject:[NSString stringWithUTF8String:VuoListGetValue_VuoText(l, i+1)]];
		return a;
	}

	if (type == "VuoColor")
	{
		VuoColor v = VuoColor_makeFromJson(vuoValue);
		CGFloat c[4] = {v.r, v.g, v.b, v.a};
		return [NSColor colorWithColorSpace:[NSColorSpace sRGBColorSpace] components:c count:4];
	}
	if (type == "VuoList_VuoColor")
	{
		VuoList_VuoColor l = VuoList_VuoColor_makeFromJson(vuoValue);
		unsigned long count = VuoListGetCount_VuoColor(l);
		NSMutableArray *a = [[NSMutableArray new] autorelease];
		for (unsigned long i=0; i<count; ++i)
		{
			VuoColor v = VuoListGetValue_VuoColor(l, i+1);
			CGFloat c[4] = {v.r, v.g, v.b, v.a};
			[a addObject:[NSColor colorWithColorSpace:[NSColorSpace sRGBColorSpace] components:c count:4]];
		}
		return a;
	}

	if (type == "VuoImage")
	{
		VuoImage v = VuoImage_makeFromJson(vuoValue);
		VuoRetain(v);
		NSImage *im = [self nsImageWithVuoImage:v];
		VuoRelease(v);
		return im;
	}
	if (type == "VuoList_VuoImage")
	{
		VuoList_VuoImage l = VuoList_VuoImage_makeFromJson(vuoValue);
		unsigned long count = VuoListGetCount_VuoImage(l);
		NSMutableArray *a = [[NSMutableArray new] autorelease];
		for (unsigned long i=0; i<count; ++i)
		{
			VuoImage v = VuoListGetValue_VuoImage(l, i+1);
			VuoRetain(v);
			[a addObject:[self nsImageWithVuoImage:v]];
			VuoRelease(v);
		}
		return a;
	}

	if (type == "VuoPoint2d")
	{
		VuoPoint2d v = VuoPoint2d_makeFromJson(vuoValue);
		return [NSValue valueWithPoint:NSMakePoint(v.x, v.y)];
	}
	if (type == "VuoList_VuoPoint2d")
	{
		VuoList_VuoPoint2d l = VuoList_VuoPoint2d_makeFromJson(vuoValue);
		unsigned long count = VuoListGetCount_VuoPoint2d(l);
		NSMutableArray *a = [[NSMutableArray new] autorelease];
		for (unsigned long i=0; i<count; ++i)
		{
			VuoPoint2d v = VuoListGetValue_VuoPoint2d(l, i+1);
			[a addObject:[NSValue valueWithPoint:NSMakePoint(v.x, v.y)]];
		}
		return a;
	}

	if (type == "VuoPoint3d")
	{
		VuoPoint3d v = VuoPoint3d_makeFromJson(vuoValue);
		size_t s = sizeof(double)*3;
		double *d = (double *)malloc(s);
		d[0] = v.x;
		d[1] = v.y;
		d[2] = v.z;
		return [NSData dataWithBytesNoCopy:d length:s];
	}
	if (type == "VuoList_VuoPoint3d")
	{
		VuoList_VuoPoint3d l = VuoList_VuoPoint3d_makeFromJson(vuoValue);
		unsigned long count = VuoListGetCount_VuoPoint3d(l);
		NSMutableArray *a = [[NSMutableArray new] autorelease];
		for (unsigned long i=0; i<count; ++i)
		{
			VuoPoint3d v = VuoListGetValue_VuoPoint3d(l, i+1);
			size_t s = sizeof(double)*3;
			double *d = (double *)malloc(s);
			d[0] = v.x;
			d[1] = v.y;
			d[2] = v.z;
			[a addObject:[NSData dataWithBytesNoCopy:d length:s]];
		}
		return a;
	}

	// Maybe it's a list of enum values?
	if (VuoStringUtilities::beginsWith(type, VuoType::listTypeNamePrefix))
	{
		string itemType = VuoStringUtilities::substrAfter(type, VuoType::listTypeNamePrefix);
		string allowedValuesFunctionName = itemType + "_getAllowedValues";
		typedef void *(*allowedValuesFunctionType)(void);
		allowedValuesFunctionType allowedValuesFunction = (allowedValuesFunctionType)dlsym(RTLD_SELF, allowedValuesFunctionName.c_str());
		if (allowedValuesFunction)
			if (json_object_is_type(vuoValue, json_type_array))
			{
				unsigned long arrayLength = json_object_array_length(vuoValue);
				if (!arrayLength)
					return [[NSArray new] autorelease];

				else if (json_object_is_type(json_object_array_get_idx(vuoValue, 0), json_type_string))
				{
					NSMutableArray *a = [[NSMutableArray new] autorelease];
					for (unsigned long i = 0; i < arrayLength; ++i)
						[a addObject:[NSString stringWithUTF8String:json_object_get_string(json_object_array_get_idx(vuoValue, i))]];
					return a;
				}
			}
	}

	// Maybe it's an enum?
	string allowedValuesFunctionName = type + "_getAllowedValues";
	typedef void *(*allowedValuesFunctionType)(void);
	allowedValuesFunctionType allowedValuesFunction = (allowedValuesFunctionType)dlsym(RTLD_SELF, allowedValuesFunctionName.c_str());
	if (allowedValuesFunction)
		if (json_object_is_type(vuoValue, json_type_string))
			return [NSString stringWithUTF8String:json_object_get_string(vuoValue)];

	VUserLog("Type %s isn't supported yet.  Please https://vuo.org/contact us if you'd like support for this type.", type.c_str());
	return nil;
}

/**
 * Does nothing.
 */
static void VuoRunnerCocoa_doNothingCallback(VuoImage imageToFree)
{
}

/**
 * Given the specified Cocoa object, returns a Vuo value (serialized as JSON).
 *
 * @see setValue:forInputPort:
 */
+ (json_object *)vuoValueWithCocoaObject:(id)value
{
	if (!value)
		return NULL;

	// Is this a Vuo-compatible NSObject (or CFType that's toll-free bridged to one)?
	if ([value respondsToSelector:@selector(vuoValue)])
		return [value vuoValue];

	// Is this a (non-toll-free bridged) CFType?
	if (strcmp(object_getClassName(value), "__NSCFType") == 0)
	{
		CFTypeID type = CFGetTypeID(value);

		if (type == CGColorGetTypeID())
		{
			NSColorSpace *nscs = [[NSColorSpace alloc] initWithCGColorSpace:CGColorGetColorSpace((CGColorRef)value)];
			NSColor *color = [NSColor colorWithColorSpace:nscs components:CGColorGetComponents((CGColorRef)value) count:CGColorGetNumberOfComponents((CGColorRef)value)];
			[nscs release];
			return [color vuoValue];
		}

		if (type == CGImageGetTypeID())
		{
			CGImageRef cgimage = (CGImageRef)value;

			if (CGImageGetAlphaInfo(cgimage) != kCGImageAlphaPremultipliedLast)
			{
				VUserLog("Error: Alpha option %d isn't supported yet.  Please use kCGImageAlphaPremultipliedLast, and https://vuo.org/contact us if you'd like support for other alpha options.", CGImageGetAlphaInfo(cgimage));
				return NULL;
			}

			if (CGImageGetBitmapInfo(cgimage) & kCGBitmapFloatComponents)
			{
				VUserLog("Error: Floating-point pixel values aren't supported yet.  Please use integer pixel values, and https://vuo.org/contact us if you'd like support for floating-point pixel values.");
				return NULL;
			}

			if (CGImageGetBitsPerComponent(cgimage) != 8)
			{
				VUserLog("Error: BitsPerComponent=%lu isn't supported yet.  Please use 8 bits per component, and https://vuo.org/contact us if you'd like support for other bit depths.", CGImageGetBitsPerComponent(cgimage));
				return NULL;
			}

			if (CGImageGetBitsPerPixel(cgimage) != 32)
			{
				VUserLog("Error: BitsPerPixel=%lu isn't supported yet.  Please use 32 bits per pixel, and https://vuo.org/contact us if you'd like support for other bit depths.", CGImageGetBitsPerPixel(cgimage));
				return NULL;
			}

			unsigned long width = CGImageGetWidth(cgimage);
			unsigned long height = CGImageGetHeight(cgimage);
			if (CGImageGetBytesPerRow(cgimage) != width*4)
			{
				VUserLog("Error: BytesPerRow must be width*4.  https://vuo.org/contact us if you'd like support for inexact strides.");
				return NULL;
			}

//			NSString *colorSpaceName = (NSString *)CGColorSpaceCopyName(CGImageGetColorSpace(cgimage));
//			if (![colorSpaceName isEqualToString:(NSString *)kCGColorSpaceSRGB])
//			{
//				VUserLog("Error: Colorspace '%s' isn't supported yet.  Please use kCGColorSpaceSRGB, and https://vuo.org/contact us if you'd like support for other colorspaces.", [colorSpaceName UTF8String]);
//				[colorSpaceName release];
//				return NULL;
//			}
//			CFRelease(colorSpaceName);

			CGDataProviderRef provider = CGImageGetDataProvider(cgimage);
			CFDataRef data = CGDataProviderCopyData(provider);

			char *bitmapData = (char *)CFDataGetBytePtr(data);
			int bytesPerRow = width*4;
			char *flippedBitmapData = (char *)malloc(bytesPerRow*height);

			// Flip the image data (CGImage is unflipped, but VuoImage_makeFromBuffer() expects flipped).
			for (unsigned long y = 0; y < height; ++y)
				memcpy(flippedBitmapData + bytesPerRow * (height - y - 1), bitmapData + bytesPerRow * y, bytesPerRow);

			VuoImage vi = VuoImage_makeFromBuffer(flippedBitmapData, GL_RGBA, width, height, VuoImageColorDepth_8, ^(void *buffer){ free(flippedBitmapData); });
			VuoRetain(vi);

			json_object *js = VuoImage_getInterprocessJson(vi);
			VuoRelease(vi);
			return js;
		}

		if (type == CVPixelBufferGetTypeID())
		{
			CVPixelBufferRef cvpb = (CVPixelBufferRef)value;

			if (CVPixelBufferIsPlanar(cvpb))
			{
				VUserLog("Error: Planar buffers aren't supported yet.  Please use chunky buffers, and https://vuo.org/contact us if you'd like support for planar buffers.");
				return NULL;
			}

			if (CVPixelBufferGetPixelFormatType(cvpb) != kCVPixelFormatType_32BGRA)
			{
				unsigned long pf = CVPixelBufferGetPixelFormatType(cvpb);
				VUserLog("Error: Pixel format %lu isn't supported yet.  Please use kCVPixelFormatType_32BGRA, and https://vuo.org/contact us if you'd like support for other pixel formats.", pf);
				return NULL;
			}

			unsigned long width = CVPixelBufferGetWidth(cvpb);
			unsigned long height = CVPixelBufferGetHeight(cvpb);
			if (CVPixelBufferGetBytesPerRow(cvpb) != width*4)
			{
				VUserLog("Error: BytesPerRow must be width*4.  https://vuo.org/contact us if you'd like support for inexact strides.");
				return NULL;
			}

			CVReturn ret = CVPixelBufferLockBaseAddress(cvpb, kCVPixelBufferLock_ReadOnly);
			if (ret != kCVReturnSuccess)
			{
				VUserLog("CVPixelBufferLockBaseAddress() failed: %d", ret);
				return NULL;
			}

			const unsigned char *pixels = (const unsigned char *)CVPixelBufferGetBaseAddress(cvpb);
			if (!pixels)
			{
				VUserLog("Error: CVPixelBufferGetBaseAddress() returned NULL.");
				return NULL;
			}

			CVPixelBufferRetain(cvpb);
			VuoImage vi = VuoImage_makeFromBuffer(pixels, GL_BGRA, width, height, VuoImageColorDepth_8, ^(void *buffer){ CVPixelBufferRelease(cvpb); });
			VuoRetain(vi);

			ret = CVPixelBufferUnlockBaseAddress(cvpb, kCVPixelBufferLock_ReadOnly);
			if (ret != kCVReturnSuccess)
			{
				VUserLog("CVPixelBufferUnlockBaseAddress() failed: %d", ret);
				return NULL;
			}

			json_object *js = VuoImage_getInterprocessJson(vi);
			VuoRelease(vi);
			return js;
		}

		if (type == CVOpenGLTextureGetTypeID())
		{
			CVOpenGLTextureRef cvgl = (CVOpenGLTextureRef)value;

			GLuint name = CVOpenGLTextureGetName(cvgl);
			if (!name)
			{
				VUserLog("Error: CVOpenGLTextureGetName() returned 0.");
				return NULL;
			}

			GLfloat lowerLeft[2];
			GLfloat lowerRight[2];
			GLfloat upperRight[2];
			GLfloat upperLeft[2];
			CVOpenGLTextureGetCleanTexCoords(cvgl, lowerLeft, lowerRight, upperRight, upperLeft);

			unsigned int width = abs((int)(lowerRight[0] - lowerLeft[0]));
			unsigned int height = abs((int)(upperLeft[1] - lowerLeft[1]));
//			VLog("%dx%d",width,height);

			VuoImage vi;
			GLenum target = CVOpenGLTextureGetTarget(cvgl);
			if (target == GL_TEXTURE_2D)
				vi = VuoImage_makeClientOwned(name, GL_RGBA, width, height, VuoRunnerCocoa_doNothingCallback, NULL);
			else if (target == GL_TEXTURE_RECTANGLE_ARB)
				vi = VuoImage_makeClientOwnedGlTextureRectangle(name, GL_RGBA, width, height, VuoRunnerCocoa_doNothingCallback, NULL);
			else
			{
				VUserLog("Error: GL texture target %d isn't supported yet.  Please use GL_TEXTURE_2D or GL_TEXTURE_RECTANGLE_ARB, and https://vuo.org/contact us if you'd like support for other targets.", target);
				return NULL;
			}

			VuoRetain(vi);
			json_object *js = VuoImage_getInterprocessJson(vi);
			VuoRelease(vi);
			return js;
		}

		NSString *typeDescription = (NSString *)CFCopyTypeIDDescription(type);
		VUserLog("Error: Unknown CFType '%s'", [typeDescription UTF8String]);
		[typeDescription release];
		return NULL;
	}

	VUserLog("Error: Unknown type '%s' for object '%s'", object_getClassName(value), [[value description] UTF8String]);
	return NULL;
}

/**
 * Converts the provided VuoImage to an NSImage.
 */
+ (NSImage *)nsImageWithVuoImage:(VuoImage)vi
{
	if (!vi)
		return nil;

	// Allocate memory to store the image data.
	unsigned int bytesPerRow = 4 * vi->pixelsWide;
	NSBitmapImageRep *nbi = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
		pixelsWide:vi->pixelsWide
		pixelsHigh:vi->pixelsHigh
		bitsPerSample:8
		samplesPerPixel:4
		hasAlpha:YES
		isPlanar:NO
		colorSpaceName:NSDeviceRGBColorSpace
		bytesPerRow:bytesPerRow
		bitsPerPixel:0];
	if (!nbi)
		return nil;

	// Download the image data from the GPU.
	unsigned char *bitmapData = [nbi bitmapData];
	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
		glBindTexture(GL_TEXTURE_2D, vi->glTextureName);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmapData);
		glBindTexture(GL_TEXTURE_2D, 0);
		VuoGlContext_disuse(cgl_ctx);
	}

	// Flip the image data (OpenGL returns flipped data, but NSBitmapImageRep assumes it is not flipped).
	unsigned char *tmp = (unsigned char *)malloc(bytesPerRow);
	for (unsigned long y = 0; y < vi->pixelsHigh / 2; ++y)
	{
		memcpy(tmp, bitmapData + bytesPerRow * y, bytesPerRow);
		memcpy(bitmapData + bytesPerRow * y, bitmapData + bytesPerRow * (vi->pixelsHigh - y - 1), bytesPerRow);
		memcpy(bitmapData + bytesPerRow * (vi->pixelsHigh - y - 1), tmp, bytesPerRow);
	}
	free(tmp);

	NSImage *ni = [[NSImage alloc] initWithSize:[nbi size]];
	[ni addRepresentation:nbi];
	[nbi release];
	return [ni autorelease];
}

@end

@implementation NSNumber (VuoRunnerCocoaConversion)
/**
 * Returns a Vuo value representation (serialized as JSON) of this Cocoa object.
 *
 * @see setValue:forInputPort:
 */
- (json_object *)vuoValue
{
	char type = [self objCType][0];

	if (type == 'B')
		return VuoBoolean_getJson([self boolValue]);

	if (type == 'f' || type == 'd')
		return VuoReal_getJson([self doubleValue]);

	if (type == 'c' || type == 'i' || type == 's' || type == 'l' || type == 'q'
	 || type == 'C' || type == 'I' || type == 'S' || type == 'L' || type == 'Q')
		return VuoInteger_getJson([self longLongValue]);

	VUserLog("Error: Unknown type '%s'", [self objCType]);
	return NULL;
}
@end

@implementation NSString (VuoRunnerCocoaConversion)
/**
 * Returns a Vuo value representation (serialized as JSON) of this Cocoa object.
 *
 * @see setValue:forInputPort:
 */
- (json_object *)vuoValue
{
	return VuoText_getJson([self UTF8String]);
}
@end

@implementation NSColor (VuoRunnerCocoaConversion)
/**
 * Returns a Vuo value representation (serialized as JSON) of this Cocoa object.
 *
 * @see setValue:forInputPort:
 */
- (json_object *)vuoValue
{
	CGFloat c[4] = {0,0,0,1};
	NSColor *color = [self colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
	[color getComponents:c];
	return VuoColor_getJson(VuoColor_makeWithRGBA(c[0], c[1], c[2], c[3]));
}
@end

@implementation CIColor (VuoRunnerCocoaConversion)
/**
 * Returns a Vuo value representation (serialized as JSON) of this Cocoa object.
 *
 * @see setValue:forInputPort:
 */
- (json_object *)vuoValue
{
	NSColorSpace *nscs = [[NSColorSpace alloc] initWithCGColorSpace:[self colorSpace]];
	NSColor *color = [NSColor colorWithColorSpace:nscs components:[self components] count:[self numberOfComponents]];
	[nscs release];
	return [color vuoValue];
}
@end

@implementation NSImage (VuoRunnerCocoaConversion)
/**
 * Returns a Vuo value representation (serialized as JSON) of this Cocoa object.
 *
 * @see setValue:forInputPort:
 */
- (json_object *)vuoValue
{
	float scale = [self recommendedLayerContentsScale:0];
	unsigned int pixelsWide = [self size].width  * scale;
	unsigned int pixelsHigh = [self size].height * scale;

	NSBitmapImageRep *nbir = [[NSBitmapImageRep alloc]
			initWithBitmapDataPlanes: NULL
			pixelsWide:pixelsWide
			pixelsHigh:pixelsHigh
			bitsPerSample:8
			samplesPerPixel:4
			hasAlpha:YES
			isPlanar:NO
			colorSpaceName:NSDeviceRGBColorSpace
			bytesPerRow:pixelsWide*4
			bitsPerPixel:32];

	NSGraphicsContext *ngc = [NSGraphicsContext graphicsContextWithBitmapImageRep:nbir];
	[NSGraphicsContext saveGraphicsState];
	[NSGraphicsContext setCurrentContext:ngc];
	[self drawInRect:NSMakeRect(0,0,pixelsWide,pixelsHigh) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1 respectFlipped:YES hints:nil];
	[ngc flushGraphics];
	[NSGraphicsContext setCurrentContext:nil];
	[NSGraphicsContext restoreGraphicsState];

	json_object *js = [nbir vuoValue];
	[nbir release];
	return js;
}
@end

@implementation NSBitmapImageRep (VuoRunnerCocoaConversion)
/**
 * Returns a Vuo value representation (serialized as JSON) of this Cocoa object.
 *
 * @see setValue:forInputPort:
 */
- (json_object *)vuoValue
{
	// Flip the image data (OpenGL expects flipped data, but NSBitmapImageRep is not flipped).
	unsigned char *bitmapData = [self bitmapData];
	unsigned long width = [self size].width;
	unsigned long height = [self size].height;
	unsigned long bytesPerRow = width * 4;
	unsigned char *bitmapDataFlipped = (unsigned char *)malloc(bytesPerRow * height);
	for (unsigned long y = 0; y < height; ++y)
		for (unsigned long x = 0; x < width; ++x)
		{
			// Swizzle RGBA -> BGRA (so the GPU doesn't have to swizzle during each fetch)
			bitmapDataFlipped[bytesPerRow * (height - y - 1) + x * 4 + 2] = bitmapData[bytesPerRow * y + x * 4 + 0];
			bitmapDataFlipped[bytesPerRow * (height - y - 1) + x * 4 + 1] = bitmapData[bytesPerRow * y + x * 4 + 1];
			bitmapDataFlipped[bytesPerRow * (height - y - 1) + x * 4 + 0] = bitmapData[bytesPerRow * y + x * 4 + 2];
			bitmapDataFlipped[bytesPerRow * (height - y - 1) + x * 4 + 3] = bitmapData[bytesPerRow * y + x * 4 + 3];
		}

	VuoImage vi = VuoImage_makeFromBuffer(bitmapDataFlipped, GL_BGRA, width, height, VuoImageColorDepth_8, ^(void *buffer){ free(buffer); });
	VuoRetain(vi);
	json_object *json = VuoImage_getInterprocessJson(vi);
	VuoRelease(vi);
	return json;
}
@end

@implementation NSValue (VuoRunnerCocoaConversion)
/**
 * Returns a Vuo value representation (serialized as JSON) of this Cocoa object.
 *
 * @see setValue:forInputPort:
 */
- (json_object *)vuoValue
{
	NSPoint p = [self pointValue];
	return VuoPoint2d_getJson(VuoPoint2d_make(p.x, p.y));
}
@end

@implementation NSData (VuoRunnerCocoaConversion)
/**
 * Returns a Vuo value representation (serialized as JSON) of this Cocoa object.
 *
 * @see setValue:forInputPort:
 */
- (json_object *)vuoValue
{
	double p[3];
	[self getBytes:p length:sizeof(double)*3];
	return VuoPoint3d_getJson(VuoPoint3d_make(p[0], p[1], p[2]));
}
@end

@implementation NSArray (VuoRunnerCocoaConversion)
/**
 * Returns a Vuo value representation (serialized as JSON) of this Cocoa object.
 *
 * @see setValue:forInputPort:
 */
- (json_object *)vuoValue
{
	json_object *arrayJson = json_object_new_array();
	for (id item in self)
	{
		json_object *itemJson = [item vuoValue];
		json_object_array_add(arrayJson, itemJson);
	}
	return arrayJson;
}
@end
