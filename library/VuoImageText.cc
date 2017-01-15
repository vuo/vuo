/**
 * @file
 * VuoImageText implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "module.h"
#include "VuoImageText.h"

#include <OpenGL/CGLMacro.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <ApplicationServices/ApplicationServices.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoImageText",
					"dependencies" : [
						"VuoImage",
						"VuoFont",
						"ApplicationServices.framework"
					]
				 });
#endif
}


#include <utility>
#include <map>
#include <string>

/**
 * A wrapper for @ref VuoFont, so it can be used as a std::map key.
 */
class VuoFontClass
{
public:
	VuoFont f;	///< The actual font.

	/**
	 * Wraps a @ref VuoFont.
	 */
	VuoFontClass(VuoFont font)
	{
		f = font;
		VuoFont_retain(f);
	}
	/**
	 * Copies a @ref VuoFontClass.
	 */
	VuoFontClass(const VuoFontClass &font)
	{
		f = font.f;
		VuoFont_retain(f);
	}
	/**
	 * Destroys a @ref VuoFontClass.
	 */
	~VuoFontClass()
	{
		VuoFont_release(f);
	}
};

/**
 * Returns true if `a` is less than `b`.
 */
bool operator<(const VuoFontClass &a, const VuoFontClass &b)
{
	return VuoFont_isLessThan(a.f, b.f);
}

typedef std::pair<std::string, std::pair<VuoFontClass,double> > VuoImageTextCacheDescriptor;	///< Text, font, and backingScaleFactor.
typedef std::pair<VuoImage,double> VuoImageTextCacheEntry;	///< An image and the last time it was used.
typedef std::map<VuoImageTextCacheDescriptor, VuoImageTextCacheEntry> VuoImageTextCacheType;	///< A pool of images.
static VuoImageTextCacheType *VuoImageTextCache;	///< A pool of images.
static dispatch_semaphore_t VuoImageTextCache_semaphore;	///< Serializes access to VuoImageTextCache.
static dispatch_semaphore_t VuoImageTextCache_canceledAndCompleted;	///< Signals when the last VuoImageTextCache cleanup has completed.
static dispatch_source_t VuoImageTextCache_timer;	///< Periodically cleans up VuoImageTextCache.
static double VuoImageTextCache_timeout = 1.0;	///< Seconds an image can remain in the cache unused, before it gets purged.

/**
 * Purges expired images from the cache.
 */
static void VuoImageTextCache_cleanup(void *blah)
{
	dispatch_semaphore_wait(VuoImageTextCache_semaphore, DISPATCH_TIME_FOREVER);
	{
		double now = VuoLogGetTime();
//		VLog("cache:");
		for (VuoImageTextCacheType::iterator item = VuoImageTextCache->begin(); item != VuoImageTextCache->end(); )
		{
			double lastUsed = item->second.second;
//			VLog("\t\"%s\" %s backingScaleFactor=%g (last used %gs ago)", item->first.first.c_str(), item->first.second.first.f.fontName, item->first.second.second, now - lastUsed);
			if (now - lastUsed > VuoImageTextCache_timeout)
			{
//				VLog("\t\tpurging");
				VuoRelease(item->second.first);
				VuoImageTextCache->erase(item++);
			}
			else
				++item;
		}
	}
	dispatch_semaphore_signal(VuoImageTextCache_semaphore);
}
/**
 * Initializes the cache.
 */
static void VuoImageTextCache_init(void)
{
	VuoImageTextCache_semaphore = dispatch_semaphore_create(1);
	VuoImageTextCache_canceledAndCompleted = dispatch_semaphore_create(0);
	VuoImageTextCache = new VuoImageTextCacheType;

	VuoImageTextCache_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
	dispatch_source_set_timer(VuoImageTextCache_timer, dispatch_walltime(NULL,0), NSEC_PER_SEC * VuoImageTextCache_timeout, NSEC_PER_SEC * VuoImageTextCache_timeout);
	dispatch_source_set_event_handler_f(VuoImageTextCache_timer, VuoImageTextCache_cleanup);
	dispatch_source_set_cancel_handler(VuoImageTextCache_timer, ^{
										   dispatch_semaphore_signal(VuoImageTextCache_canceledAndCompleted);
									   });
	dispatch_resume(VuoImageTextCache_timer);
}
/**
 * Destroys the cache.
 */
static void VuoImageTextCache_fini(void)
{
	dispatch_source_cancel(VuoImageTextCache_timer);

	// Wait for the last cleanup to complete.
	dispatch_semaphore_wait(VuoImageTextCache_canceledAndCompleted, DISPATCH_TIME_FOREVER);

	// Clean up anything that still remains.
//	VLog("cache:");
	for (VuoImageTextCacheType::iterator item = VuoImageTextCache->begin(); item != VuoImageTextCache->end(); ++item)
	{
//		VLog("\t\"%s\" %s backingScaleFactor=%g", item->first.first.c_str(), item->first.second.first.f.fontName, item->first.second.second);
//		VLog("\t\tpurging");
		VuoRelease(item->second.first);
	}

	delete VuoImageTextCache;
}


/**
 * Formats the specified `text` for rendering using `font` and `backingScaleFactor`.
 */
static CFArrayRef VuoImageText_createCTLines(VuoText text, VuoFont font, float backingScaleFactor, CTFontRef *ctFont, CGColorRef *cgColor, CGColorSpaceRef *colorspace, unsigned int *width, unsigned int *height, double *lineHeight, CGRect *bounds, CGRect **lineBounds)
{
	CFStringRef fontNameCF = NULL;
	if (font.fontName)
		fontNameCF = CFStringCreateWithCString(NULL, font.fontName, kCFStringEncodingUTF8);

	*ctFont = CTFontCreateWithName(fontNameCF ? fontNameCF : CFSTR(""), font.pointSize * backingScaleFactor, NULL);

	if (fontNameCF)
		CFRelease(fontNameCF);

	*cgColor = CGColorCreateGenericRGB(font.color.r, font.color.g, font.color.b, font.color.a);

	unsigned long underline = font.underline ? kCTUnderlineStyleSingle : kCTUnderlineStyleNone;
	CFNumberRef underlineNumber = CFNumberCreate(NULL, kCFNumberCFIndexType, &underline);

	float kern = (font.characterSpacing-1) * font.pointSize * backingScaleFactor;
	CFNumberRef kernNumber = CFNumberCreate(NULL, kCFNumberFloatType, &kern);

	// Create a temporary context to get the bounds.
	*colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	CGContextRef cgContext = CGBitmapContextCreate(NULL, 1, 1, 8, 4, *colorspace, kCGImageAlphaPremultipliedLast);

	// Split the user's text into lines.
	CFStringRef cfText = CFStringCreateWithCStringNoCopy(NULL, text, kCFStringEncodingUTF8, kCFAllocatorNull);
	CFArrayRef lines = CFStringCreateArrayBySeparatingStrings(NULL, cfText, CFSTR("\n"));
	CFIndex lineCount = CFArrayGetCount(lines);

	// Create an attributed string and CTLine for each line of text, specifying the font, color, underline.
	CFMutableArrayRef attributedLines = CFArrayCreateMutable(NULL, lineCount, &kCFTypeArrayCallBacks);
	CFMutableArrayRef ctLines = CFArrayCreateMutable(NULL, lineCount, &kCFTypeArrayCallBacks);
	for (CFIndex i=0; i<lineCount; ++i)
	{
		CFStringRef line = (CFStringRef)CFArrayGetValueAtIndex(lines, i);

		CFStringRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName, kCTUnderlineStyleAttributeName, kCTKernAttributeName };
		CFTypeRef values[] = { *ctFont, *cgColor, underlineNumber, kernNumber };
		CFDictionaryRef attr = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values, sizeof(keys) / sizeof(keys[0]), NULL, NULL);
		CFAttributedStringRef attributedLine = CFAttributedStringCreate(NULL, line, attr);
		CFRelease(attr);
		CFArrayAppendValue(attributedLines, attributedLine);

		CTLineRef ctLine = CTLineCreateWithAttributedString(attributedLine);
		CFArrayAppendValue(ctLines, ctLine);
		CFRelease(ctLine);
		CFRelease(attributedLine);
	}

	// Get the bounds of each line of text, and union them into bounds for the entire block of text.
	double ascent = CTFontGetAscent(*ctFont);
	double descent = CTFontGetDescent(*ctFont);
	double leading = CTFontGetLeading(*ctFont);
	*lineHeight = ascent + descent + leading;
	*bounds = CGRectMake(0,0,0,0);
	*lineBounds = (CGRect *)malloc(sizeof(CGRect) * lineCount);
	for (CFIndex i=0; i<lineCount; ++i)
	{
		CTLineRef ctLine = (CTLineRef)CFArrayGetValueAtIndex(ctLines, i);

		// For some fonts (such as Consolas), CTLineGetImageBounds doesn't return sufficient bounds --- the right side of the text gets cut off.
		// For other fonts (such as Zapfino), CTLineGetTypographicBounds doesn't return sufficient bounds --- the left side of its loopy "g" gets cut off.
		// So combine the results of both.
		double width = CTLineGetTypographicBounds(ctLine, NULL, NULL, NULL);
		CGRect lineImageBounds = CTLineGetImageBounds(ctLine, cgContext);
		width = fmax(width,CGRectGetWidth(lineImageBounds));
		width += CTLineGetTrailingWhitespaceWidth(ctLine);
		(*lineBounds)[i] = CGRectMake(CGRectGetMinX(lineImageBounds), (*lineHeight) * i - ascent, width, *lineHeight);

		// Can't use CGRectUnion since it shifts the origin to (0,0), cutting off the glyph's ascent and strokes left of the origin (e.g., Zapfino's "g").
		if (CGRectGetMinX((*lineBounds)[i]) < CGRectGetMinX(*bounds))
			bounds->origin.x = CGRectGetMinX((*lineBounds)[i]);
		if (CGRectGetMinY((*lineBounds)[i]) < CGRectGetMinY(*bounds))
			bounds->origin.y = CGRectGetMinY((*lineBounds)[i]);
		if (CGRectGetWidth((*lineBounds)[i]) > CGRectGetWidth(*bounds))
			bounds->size.width = CGRectGetWidth((*lineBounds)[i]);

		// Final bounds should always include the full first line's height.
		if (i==0)
			bounds->size.height += *lineHeight;
		else
			bounds->size.height += *lineHeight * font.lineSpacing;
	}

	// The 2 extra pixels are to account for the antialiasing on strokes that touch the edge of the glyph bounds — without those pixels, some edge strokes are slightly cut off.
	*width = ceil(CGRectGetWidth(*bounds))+2;
	*height = ceil(CGRectGetHeight(*bounds))+2;

	// Release the temporary context.
	CGContextRelease(cgContext);

	CFRelease(attributedLines);
	CFRelease(lines);
	CFRelease(cfText);
	CFRelease(kernNumber);
	CFRelease(underlineNumber);

	return ctLines;
}

/**
 * Returns a rectangle (in points) that fully encloses the specified `text` when rendered with the specified `font`.
 *
 * Depending on the font's design (e.g., if the font is monospace), the rectangle may be larger than the actual glyphs.
 */
VuoRectangle VuoImage_getTextRectangle(VuoText text, VuoFont font)
{
	if (!VuoText_length(text))
		return VuoRectangle_make(0,0,0,0);

	CTFontRef ctFont;
	CGColorRef cgColor;
	CGColorSpaceRef colorspace;
	unsigned int width;
	unsigned int height;
	double lineHeight;
	CGRect bounds;
	CGRect *lineBounds;
	CFArrayRef ctLines = VuoImageText_createCTLines(text, font, 1, &ctFont, &cgColor, &colorspace, &width, &height, &lineHeight, &bounds, &lineBounds);
	CFRelease(ctLines);
	CGColorRelease(cgColor);
	CGColorSpaceRelease(colorspace);
	CFRelease(ctFont);
	free(lineBounds);

	return VuoRectangle_make(0, 0, width, height);
}

/**
 * Creates an image containing the specified text.
 */
VuoImage VuoImage_makeText(VuoText text, VuoFont font, float backingScaleFactor)
{
	if (!text || text[0] == 0)
		return NULL;


	// Is there an image ready in the cache?
	static dispatch_once_t initCache = 0;
	dispatch_once(&initCache, ^{
					  VuoImageTextCache_init();
					  VuoAddCompositionFiniCallback(VuoImageTextCache_fini);
				  });
	VuoFontClass fc(font);
	VuoImageTextCacheDescriptor descriptor(text, std::make_pair(font, backingScaleFactor));
	dispatch_semaphore_wait(VuoImageTextCache_semaphore, DISPATCH_TIME_FOREVER);
	{
		VuoImageTextCacheType::iterator e = VuoImageTextCache->find(descriptor);
		if (e != VuoImageTextCache->end())
		{
//			VLog("found in cache");
			VuoImage image = e->second.first;
			e->second.second = VuoLogGetTime();
			dispatch_semaphore_signal(VuoImageTextCache_semaphore);
			return image;
		}
	}
	dispatch_semaphore_signal(VuoImageTextCache_semaphore);


	// …if not, render it.

	CTFontRef ctFont;
	CGColorRef cgColor;
	CGColorSpaceRef colorspace;
	unsigned int width;
	unsigned int height;
	double lineHeight;
	CGRect bounds;
	CGRect *lineBounds;
	CFArrayRef ctLines = VuoImageText_createCTLines(text, font, backingScaleFactor, &ctFont, &cgColor, &colorspace, &width, &height, &lineHeight, &bounds, &lineBounds);

	// Create the rendering context.
	// VuoImage_makeFromBuffer() expects a premultiplied buffer.
	CGContextRef cgContext = CGBitmapContextCreate(NULL, width, height, 8, width * 4, colorspace, kCGImageAlphaPremultipliedLast);

	// Vertically flip, since VuoImage_makeFromBuffer() expects a flipped buffer.
	CGContextSetTextMatrix(cgContext, CGAffineTransformMakeScale(1.0, -1.0));

	// Draw each line of text.
	CFIndex lineCount = CFArrayGetCount(ctLines);
	for (CFIndex i = 0; i < lineCount; ++i)
	{
		CTLineRef ctLine = (CTLineRef)CFArrayGetValueAtIndex(ctLines, i);

		float textXPosition = 1 - CGRectGetMinX(bounds);
		if (font.alignment == VuoHorizontalAlignment_Center)
			textXPosition += (CGRectGetWidth(bounds) - CGRectGetWidth(lineBounds[i]))/2.;
		else if (font.alignment == VuoHorizontalAlignment_Right)
			textXPosition += CGRectGetWidth(bounds) - CGRectGetWidth(lineBounds[i]);

		float textYPosition = 1 - CGRectGetMinY(bounds);
		textYPosition += lineHeight * i * font.lineSpacing;

		CGContextSetTextPosition(cgContext, textXPosition, textYPosition);
		CTLineDraw(ctLine, cgContext);
	}

	// Make a VuoImage from the CGContext.
	VuoImage image = VuoImage_makeFromBuffer(CGBitmapContextGetData(cgContext), GL_RGBA, width, height, VuoImageColorDepth_8, ^(void *buffer){ CGContextRelease(cgContext); });

	CFRelease(ctLines);
	CGColorSpaceRelease(colorspace);
	CGColorRelease(cgColor);
	CFRelease(ctFont);
	free(lineBounds);


	// …and store it in the cache.
	dispatch_semaphore_wait(VuoImageTextCache_semaphore, DISPATCH_TIME_FOREVER);
	{
		VuoRetain(image);
		VuoImageTextCacheEntry e(image, VuoLogGetTime());
		(*VuoImageTextCache)[descriptor] = e;
//		VLog("stored in cache");
	}
	dispatch_semaphore_signal(VuoImageTextCache_semaphore);


	return image;
}
