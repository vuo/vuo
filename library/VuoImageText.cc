/**
 * @file
 * VuoImageText implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "module.h"
#include "VuoImageText.h"
#include "VuoEventLoop.h"

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
#include <vector>

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
	VuoFontClass(VuoFont font) :
		f(font)
	{
		VuoFont_retain(f);
	}
	/**
	 * Copies a @ref VuoFontClass.
	 */
	VuoFontClass(const VuoFontClass &font) :
		f(font.f)
	{
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

/**
 * Initialize a new instance of VuoImageTextData.
 */
VuoImageTextData VuoImageTextData_make()
{
	VuoImageTextData i = (VuoImageTextData) malloc(sizeof(struct _VuoImageTextData));
	i->lineCounts = NULL;
	i->lineBounds = NULL;
	i->charAdvance = NULL;
	VuoRegister(i, VuoImageTextData_free);
	return i;
}

/**
 * Free a VuoImageTextData pointer and it's contents.
 */
void VuoImageTextData_free(void* data)
{
	VuoImageTextData value = (VuoImageTextData) data;
	if(value->lineCounts) free(value->lineCounts);
	if(value->lineBounds) free(value->lineBounds);
	if(value->charAdvance) free(value->charAdvance);
	free(value);
}

typedef std::pair<std::string, std::pair<VuoFontClass, double> > VuoImageTextCacheDescriptor;	///< Text, font, and backingScaleFactor.
typedef std::pair<VuoImage, double> VuoImageTextCacheEntry;	///< An image and the last time it was used.
typedef std::map<VuoImageTextCacheDescriptor, VuoImageTextCacheEntry> VuoImageTextCacheType;	///< A pool of images.
static VuoImageTextCacheType *VuoImageTextCache;	///< A pool of images.
static dispatch_semaphore_t VuoImageTextCache_semaphore;	///< Serializes access to VuoImageTextCache.
static dispatch_semaphore_t VuoImageTextCache_canceledAndCompleted;     ///< Signals when the last VuoImageTextCache cleanup has completed.
static dispatch_source_t VuoImageTextCache_timer = NULL;	///< Periodically cleans up VuoImageTextCache.
static double VuoImageTextCache_timeout = 1.0;	///< Seconds an image can remain in the cache unused, before it gets purged.

/**
 * Purges expired images from the cache.
 */
static void VuoImageTextCache_cleanup(void *blah)
{
	std::vector<VuoImage> imagesToRelease;

	dispatch_semaphore_wait(VuoImageTextCache_semaphore, DISPATCH_TIME_FOREVER);
	{
		double now = VuoLogGetTime();
		// VLog("cache:");
		for (VuoImageTextCacheType::iterator item = VuoImageTextCache->begin(); item != VuoImageTextCache->end(); )
		{
			double lastUsed = item->second.second;
			// VLog("\t\"%s\" %s backingScaleFactor=%g (last used %gs ago)", item->first.first.c_str(), item->first.second.first.f.fontName, item->first.second.second, now - lastUsed);
			if (now - lastUsed > VuoImageTextCache_timeout)
			{
				// VLog("\t\tpurging");
				imagesToRelease.push_back(item->second.first);
				VuoImageTextCache->erase(item++);
			}
			else
				++item;
		}
	}
	dispatch_semaphore_signal(VuoImageTextCache_semaphore);

	for (std::vector<VuoImage>::iterator item = imagesToRelease.begin(); item != imagesToRelease.end(); ++item)
		VuoRelease(*item);
}

/**
 * Initializes the cache.
 */
static void VuoImageTextCache_init(void)
{
	VuoImageTextCache_semaphore = dispatch_semaphore_create(1);
	VuoImageTextCache_canceledAndCompleted = dispatch_semaphore_create(0);
	VuoImageTextCache = new VuoImageTextCacheType;

	VuoImageTextCache_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, VuoEventLoop_getDispatchStrictMask(), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
	dispatch_source_set_timer(VuoImageTextCache_timer, dispatch_walltime(NULL, 0), NSEC_PER_SEC * VuoImageTextCache_timeout, NSEC_PER_SEC * VuoImageTextCache_timeout);
	dispatch_source_set_event_handler_f(VuoImageTextCache_timer, VuoImageTextCache_cleanup);
	dispatch_source_set_cancel_handler(VuoImageTextCache_timer, ^ {
					dispatch_semaphore_signal(VuoImageTextCache_canceledAndCompleted);
			});
	dispatch_resume(VuoImageTextCache_timer);
}

/**
 * Destroys the cache.
 *
 * This is called by @ref VuoRuntimeState::stopCompositionAsOrderedByRunner.
 */
extern "C" void __attribute__((destructor)) VuoImageTextCache_fini(void)
{
	if (! VuoImageTextCache_timer)
		return;

	dispatch_source_cancel(VuoImageTextCache_timer);

	// Wait for the last cleanup to complete.
	dispatch_semaphore_wait(VuoImageTextCache_canceledAndCompleted, DISPATCH_TIME_FOREVER);

	// Clean up anything that still remains.
	//      VLog("cache:");
	for (VuoImageTextCacheType::iterator item = VuoImageTextCache->begin(); item != VuoImageTextCache->end(); ++item)
	{
		// VLog("\t\"%s\" %s backingScaleFactor=%g", item->first.first.c_str(), item->first.second.first.f.fontName, item->first.second.second);
		// VLog("\t\tpurging");
		VuoRelease(item->second.first);
	}

	delete VuoImageTextCache;
	dispatch_release(VuoImageTextCache_timer);
	VuoImageTextCache_timer = NULL;
}

/**
 * Formats the specified `text` for rendering using `font` and `backingScaleFactor`.
 */
static CFArrayRef VuoImageText_createCTLines(
	VuoText text,
	VuoFont font,
	float backingScaleFactor,
	bool includeTrailingWhiteSpace,
	CTFontRef *ctFont,
	CGColorRef *cgColor,
	CGColorSpaceRef *colorspace,
	VuoImageTextData textImageData)
{
	CFStringRef fontNameCF = NULL;

	if (font.fontName)
		fontNameCF = CFStringCreateWithCString(NULL, font.fontName, kCFStringEncodingUTF8);

	*ctFont = CTFontCreateWithName(fontNameCF ? fontNameCF : CFSTR(""), font.pointSize * backingScaleFactor, NULL);

	if (fontNameCF)
		CFRelease(fontNameCF);

	CGFloat colorComponents[4] = {font.color.r, font.color.g, font.color.b, font.color.a};
	*colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	*cgColor = CGColorCreate(*colorspace, colorComponents);

	unsigned long underline = font.underline ? kCTUnderlineStyleSingle : kCTUnderlineStyleNone;
	CFNumberRef underlineNumber = CFNumberCreate(NULL, kCFNumberCFIndexType, &underline);

	float kern = (font.characterSpacing - 1) * font.pointSize * backingScaleFactor;
	CFNumberRef kernNumber = CFNumberCreate(NULL, kCFNumberFloatType, &kern);

	// Create a temporary context to get the bounds.
	CGContextRef cgContext = CGBitmapContextCreate(NULL, 1, 1, 8, 4, *colorspace, kCGImageAlphaPremultipliedLast);

	// Split the user's text into lines.
	CFStringRef cfText = CFStringCreateWithCStringNoCopy(NULL, text, kCFStringEncodingUTF8, kCFAllocatorNull);
	CFIndex characterCount = CFStringGetLength(cfText);
	CFArrayRef lines = CFStringCreateArrayBySeparatingStrings(NULL, cfText, CFSTR("\n"));
	CFIndex lineCount = CFArrayGetCount(lines);

	// Create an attributed string and CTLine for each line of text, specifying the font, color, underline.
	CFMutableArrayRef attributedLines = CFArrayCreateMutable(NULL, lineCount, &kCFTypeArrayCallBacks);
	CFMutableArrayRef ctLines = CFArrayCreateMutable(NULL, lineCount, &kCFTypeArrayCallBacks);

	for (CFIndex i = 0; i < lineCount; ++i)
	{
		CFStringRef lineText = (CFStringRef)CFArrayGetValueAtIndex(lines, i);

		CFStringRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName, kCTUnderlineStyleAttributeName, kCTKernAttributeName };
		CFTypeRef values[] = { *ctFont, *cgColor, underlineNumber, kernNumber };
		CFDictionaryRef attr = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values, sizeof(keys) / sizeof(keys[0]), NULL, NULL);
		CFAttributedStringRef attributedLine = CFAttributedStringCreate(NULL, lineText, attr);
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

	double lineHeight = ascent + descent + leading;
	CGRect bounds = CGRectMake(0, 0, 0, 0);
	CGRect* lineBounds = (CGRect *)malloc(sizeof(CGRect) * lineCount);
	unsigned int* lineCounts = (unsigned int*) malloc(sizeof(unsigned int) * lineCount);
	VuoReal* charAdvance = (VuoReal*) malloc(sizeof(VuoReal) * characterCount);
	long charIndex = 0;
	long counted = 0;

	for (CFIndex i = 0; i < lineCount; ++i)
	{
		CTLineRef ctLine = (CTLineRef)CFArrayGetValueAtIndex(ctLines, i);
		CFIndex charLen = CTLineGetStringRange(ctLine).length;
		lineCounts[i] = charLen;
		counted += charLen;

		// get each individual character offset
		CGFloat secondaryOffset;
		float previousAdvance = CTLineGetOffsetForStringIndex (ctLine, 0, &secondaryOffset );

		for (int n = 1; n < charLen; n++)
		{
			CGFloat offset = CTLineGetOffsetForStringIndex (ctLine, n, &secondaryOffset );
			charAdvance[charIndex++] = (VuoReal)offset - previousAdvance;
			previousAdvance = (VuoReal)offset;
		}

		// For some fonts (such as Consolas), CTLineGetImageBounds doesn't return sufficient bounds --- the right side of the text gets cut off.
		// For other fonts (such as Zapfino), CTLineGetTypographicBounds doesn't return sufficient bounds --- the left side of its loopy "g" gets cut off.
		// So combine the results of both.
		double width = CTLineGetTypographicBounds(ctLine, NULL, NULL, NULL);
		CGRect lineImageBounds = CTLineGetImageBounds(ctLine, cgContext);
		width = fmax(width, CGRectGetWidth(lineImageBounds));
		if (includeTrailingWhiteSpace)
			width += CTLineGetTrailingWhitespaceWidth(ctLine);
		lineBounds[i] = CGRectMake(CGRectGetMinX(lineImageBounds), lineHeight * i - ascent, width, lineHeight);

		// Can't use CGRectUnion since it shifts the origin to (0,0), cutting off the glyph's ascent and strokes left of the origin (e.g., Zapfino's "g").
		if (CGRectGetMinX(lineBounds[i]) < CGRectGetMinX(bounds))
			bounds.origin.x = CGRectGetMinX(lineBounds[i]);
		if (CGRectGetMinY(lineBounds[i]) < CGRectGetMinY(bounds))
			bounds.origin.y = CGRectGetMinY(lineBounds[i]);
		if (CGRectGetWidth(lineBounds[i]) > CGRectGetWidth(bounds))
			bounds.size.width = CGRectGetWidth(lineBounds[i]);

		// last character in line is width - prev. advance
		if (charLen > 0)
			charAdvance[charIndex++] = width - previousAdvance;

		// if this is not the last line, add 0 for new line char width,
		// unless there weren't any characters in this line (\n\n for example
		// should only have 2 characters in the charAdvance array)
		if (i < lineCount - 1)
			charAdvance[charIndex++] = 0;

		// Final bounds should always include the full first line's height.
		if (i == 0)
			bounds.size.height += lineHeight;
		else
			bounds.size.height += lineHeight * font.lineSpacing;
	}

	// The 2 extra pixels are to account for the antialiasing on strokes that touch the edge of the glyph bounds — without those pixels, some edge strokes are slightly cut off.
	unsigned int width = ceil(CGRectGetWidth(bounds)) + 2;
	unsigned int height = ceil(CGRectGetHeight(bounds)) + 2;

	textImageData->width = width;
	textImageData->height = height;
	textImageData->lineHeight = lineHeight;
	textImageData->bounds = VuoRectangle_make(CGRectGetMidX(bounds), CGRectGetMidY(bounds), CGRectGetWidth(bounds), CGRectGetHeight(bounds));
	textImageData->lineCount = lineCount;
	textImageData->lineCounts = lineCounts;
	textImageData->lineBounds = (VuoRectangle*) malloc(sizeof(VuoRectangle) * lineCount);
	textImageData->charAdvance = charAdvance;
	textImageData->charCount = charIndex;
	textImageData->horizontalAlignment = font.alignment;

	for (int i = 0; i < lineCount; i++)
		textImageData->lineBounds[i] = VuoRectangle_make(CGRectGetMidX(lineBounds[i]), CGRectGetMidY(lineBounds[i]), CGRectGetWidth(lineBounds[i]), CGRectGetHeight(lineBounds[i]));

	free(lineBounds);

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
VuoRectangle VuoImage_getTextRectangle(VuoText text, VuoFont font, bool includeTrailingWhiteSpace)
{
	VuoImageTextData data = VuoImage_getTextImageData(text, font, includeTrailingWhiteSpace);

	if (data == NULL)
		return VuoRectangle_make(0, 0, 0, 0);

	VuoLocal(data);

	return VuoRectangle_make(0, 0, data->width, data->height);
}

/**
 * Returns a struct containing all information necessary to calculate text and character size and placement when rendered.
 *
 * If text is null or empty, null is returned.
 */
VuoImageTextData VuoImage_getTextImageData(VuoText text, VuoFont font, bool includeTrailingWhiteSpace)
{
	if (!VuoText_length(text))
		return NULL;

	CTFontRef ctFont;
	CGColorRef cgColor;
	CGColorSpaceRef colorspace;
	VuoImageTextData textData = VuoImageTextData_make();
	CFArrayRef ctLines = VuoImageText_createCTLines(text, font, 1, includeTrailingWhiteSpace, &ctFont, &cgColor, &colorspace, textData);
	CFRelease(ctLines);
	CGColorRelease(cgColor);
	CGColorSpaceRelease(colorspace);
	CFRelease(ctFont);

	return textData;
}

/**
 * Outputs the size in Vuo coordinates of a real-size text layer.
 * windowSize is in points.
 */
VuoPoint2d VuoImageText_getTextSize(VuoText text, VuoFont font, VuoPoint2d windowSize, VuoReal backingScaleFactor, bool includeTrailingWhiteSpace)
{
	VuoRectangle textBounds = VuoImage_getTextRectangle(text, font, includeTrailingWhiteSpace);

	float w = textBounds.size.x * backingScaleFactor;
	float h = textBounds.size.y * backingScaleFactor;

	VuoPoint2d size;
	size.x = (w / windowSize.x) * 2;
	size.y = size.x * (h / w);

	return size;
}

#define ScreenToGL(x) (((x * backingScaleFactor) / screenWidthInPixels) * 2.)	///< Shortcut macro for converting to GL coordinates in VuoImageTextData_convertToVuoCoordinates function.

/**
 * Convert VuoImageTextData from pixel coordinates to Vuo coordinates in place.
 */
void VuoImageTextData_convertToVuoCoordinates(VuoImageTextData textData, VuoReal screenWidthInPixels, VuoReal backingScaleFactor)
{
	// @todo convert the other values too
	double w = textData->width * backingScaleFactor;
	double h = textData->height * backingScaleFactor;

	textData->width = (w / screenWidthInPixels) * 2;
	textData->height = textData->width * (h / w);

	textData->lineHeight = ScreenToGL(textData->lineHeight);

	textData->bounds.center = VuoPoint2d_make(ScreenToGL(textData->bounds.center.x), ScreenToGL(textData->bounds.center.y));
	textData->bounds.size  = VuoPoint2d_make(ScreenToGL(textData->bounds.size.x), ScreenToGL(textData->bounds.size.y));

	for (int i = 0; i < textData->lineCount; i++)
	{
		textData->lineBounds[i].center = VuoPoint2d_make(ScreenToGL(textData->lineBounds[i].center.x), ScreenToGL(textData->lineBounds[i].center.y));
		textData->lineBounds[i].size = VuoPoint2d_make(ScreenToGL(textData->lineBounds[i].size.x), ScreenToGL(textData->lineBounds[i].size.y));
	}

	for (int n = 0; n < textData->charCount; n++)
	{
		float advance = textData->charAdvance[n];
		textData->charAdvance[n] = ScreenToGL(advance);
	}
}

#undef ScreenToGL

/**
 * Get the origin point of a line of text.
 */
VuoPoint2d VuoImageTextData_getOriginForLineIndex(VuoImageTextData textData, unsigned int lineIndex)
{
	double x = 0;

	// if horizontal alignment is left the origin is just image bounds left.
	// if center or right, the start position is line width dependent
	if(textData->horizontalAlignment == VuoHorizontalAlignment_Left)
	{
		x = -textData->width * .5;
	}
	else
	{
		if(textData->horizontalAlignment == VuoHorizontalAlignment_Center)
			x = textData->lineBounds[lineIndex].size.x * -.5;
		else
			x = textData->width * .5 - textData->lineBounds[lineIndex].size.x;
	}

	// position is bottom left of rect (includes descent)
	return VuoPoint2d_make(x, ((textData->lineCount - lineIndex) * textData->lineHeight) - textData->lineHeight - (textData->height * .5));
}

/**
 * Get the starting point for a character index (bottom left corner).
 */
VuoPoint2d VuoImageTextData_getPositionForCharIndex(VuoImageTextData textData, unsigned int charIndex)
{
	charIndex = MAX(0, MIN(textData->charCount, charIndex));

	// figure out which row of text the char index is on
	unsigned int lineIndex = 0;

	// the char index that the starting line begins with
	unsigned int lineStart = 0;

	while( charIndex > lineStart + textData->lineCounts[lineIndex])
	{
		// add 1 because lineCounts does not include the \n char
		lineStart += textData->lineCounts[lineIndex] + 1;
		lineIndex++;
	}

	VuoPoint2d pos = VuoImageTextData_getOriginForLineIndex(textData, lineIndex);

	// now that origin x and y are set, step through line to actual index
	for(int n = lineStart; n < charIndex; n++)
		pos.x += textData->charAdvance[n];

	return pos;
}

/**
 * Return the character index nearest to point (0 indexed).
 */
int VuoImageTextData_getNearestCharToPoint(VuoImageTextData textData, VuoPoint2d point)
{
	int index = 0;

	for(int r = 0; r < textData->lineCount; r++)
	{
		bool lastLine = r == textData->lineCount - 1;
		VuoRectangle lineBounds = textData->lineBounds[r];
		VuoPoint2d lineOrigin = VuoImageTextData_getOriginForLineIndex(textData, r);

		// if the point is above this line, or it's the last line, that means
		// this row is the nearest to  the cursor
		if(point.y > lineOrigin.y || lastLine)
		{
			// left of the start of this line, so first index it is
			if(point.x < lineOrigin.x)
				return index;
			// right of the end of this line, so last index (before new line char) unless it's the last line
			else if(point.x > (lineOrigin.x + lineBounds.size.x))
				return index + textData->lineCounts[r] + (lastLine ? 1 : 0);

			for(int i = 0; i < textData->lineCounts[r]; i++)
			{
				float advance = textData->charAdvance[index + i];

				if(point.x < lineOrigin.x + advance * .5)
					return index + i;

				lineOrigin.x += advance;
			}

			return index + textData->lineCounts[r] + (lastLine ? 1 : 0);
		}

		index += textData->lineCounts[r] + 1;
	}

	// never should get here, but just in case
	return textData->charCount;
}

/**
 * Creates an image containing the specified text.
 */
VuoImage VuoImage_makeText(VuoText text, VuoFont font, float backingScaleFactor)
{
	if (VuoText_isEmpty(text))
		return NULL;

	// Is there an image ready in the cache?
	static dispatch_once_t initCache = 0;
	dispatch_once(&initCache, ^ {
		VuoImageTextCache_init();
	});
	VuoFontClass fc(font);
	VuoImageTextCacheDescriptor descriptor(text, std::make_pair(font, backingScaleFactor));
	dispatch_semaphore_wait(VuoImageTextCache_semaphore, DISPATCH_TIME_FOREVER);
	{
		VuoImageTextCacheType::iterator e = VuoImageTextCache->find(descriptor);
		if (e != VuoImageTextCache->end())
		{
			// VLog("found in cache");
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

	VuoImageTextData textData = VuoImageTextData_make();
	VuoLocal(textData);
	CFArrayRef ctLines = VuoImageText_createCTLines(text, font, backingScaleFactor, true, &ctFont, &cgColor, &colorspace, textData);

	// Create the rendering context.
	// VuoImage_makeFromBuffer() expects a premultiplied buffer.
	CGContextRef cgContext = CGBitmapContextCreate(NULL, textData->width, textData->height, 8, textData->width * 4, colorspace, kCGImageAlphaPremultipliedLast);

	// Vertically flip, since VuoImage_makeFromBuffer() expects a flipped buffer.
	CGContextSetTextMatrix(cgContext, CGAffineTransformMakeScale(1.0, -1.0));

	VuoRectangle bounds = textData->bounds;

	// Draw each line of text.
	CFIndex lineCount = CFArrayGetCount(ctLines);
	for (CFIndex i = 0; i < lineCount; ++i)
	{
		CTLineRef ctLine = (CTLineRef)CFArrayGetValueAtIndex(ctLines, i);

		float textXPosition = 1 - (bounds.center.x - (bounds.size.x * .5));
		if (font.alignment == VuoHorizontalAlignment_Center)
			textXPosition += (bounds.size.x - textData->lineBounds[i].size.x) / 2.;
		else if (font.alignment == VuoHorizontalAlignment_Right)
			textXPosition += bounds.size.x - textData->lineBounds[i].size.x;

		float textYPosition = 1 - (bounds.center.y - (bounds.size.y * .5));
		textYPosition += textData->lineHeight * i * font.lineSpacing;

		CGContextSetTextPosition(cgContext, textXPosition, textYPosition);
		CTLineDraw(ctLine, cgContext);
	}

	// Make a VuoImage from the CGContext.
	VuoImage image = VuoImage_makeFromBuffer(CGBitmapContextGetData(cgContext), GL_RGBA, textData->width, textData->height, VuoImageColorDepth_8, ^(void *buffer) { CGContextRelease(cgContext); });

	CFRelease(ctLines);
	CGColorSpaceRelease(colorspace);
	CGColorRelease(cgColor);
	CFRelease(ctFont);

	// …and store it in the cache.
	if (image)
	{
		dispatch_semaphore_wait(VuoImageTextCache_semaphore, DISPATCH_TIME_FOREVER);

		// Another thread might have added it to the cache since the check at the beginning of this function.
		VuoImageTextCacheType::iterator it = VuoImageTextCache->find(descriptor);
		if (it != VuoImageTextCache->end())
		{
//			VLog("This image is already in the cache; I'm destroying mine and returning the cached image.");
			VuoLocal(image);

			VuoImage cachedImage = it->second.first;
			it->second.second = VuoLogGetTime();
			dispatch_semaphore_signal(VuoImageTextCache_semaphore);
			return cachedImage;
		}

		VuoRetain(image);
		VuoImageTextCacheEntry e(image, VuoLogGetTime());
		(*VuoImageTextCache)[descriptor] = e;
		// VLog("stored in cache");

		dispatch_semaphore_signal(VuoImageTextCache_semaphore);
	}


	return image;
}
