/**
 * @file
 * VuoImageText implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "module.h"
#include "VuoImageText.h"

#include <OpenGL/CGLMacro.h>

#include <ApplicationServices/ApplicationServices.h>

extern "C" {
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
	float backingScaleFactor; ///< 1 = nonretina, 2 = retina.
	float verticalScale;      ///< 1 = normal font aspect ratio.
	float rotation;           ///< In radians.
	float wrapWidth;          ///< The width (in Vuo Coordinates) at which to wrap lines, or infinity to disable wrapping.

	/**
	 * Wraps a @ref VuoFont.
	 */
	VuoFontClass(VuoFont font, float backingScaleFactor, float verticalScale, float rotation, float wrapWidth) :
		f(font), backingScaleFactor(backingScaleFactor), verticalScale(verticalScale), rotation(rotation), wrapWidth(wrapWidth)
	{
		VuoFont_retain(f);
	}
	/**
	 * Copies a @ref VuoFontClass.
	 */
	VuoFontClass(const VuoFontClass &font) :
		f(font.f), backingScaleFactor(font.backingScaleFactor), verticalScale(font.verticalScale), rotation(font.rotation), wrapWidth(font.wrapWidth)
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
	VuoType_returnInequality(VuoFont, a.f, b.f);
	VuoType_returnInequality(VuoReal, a.backingScaleFactor, b.backingScaleFactor);
	VuoType_returnInequality(VuoReal, a.verticalScale, b.verticalScale);
	VuoType_returnInequality(VuoReal, a.rotation, b.rotation);
	VuoType_returnInequality(VuoReal, a.wrapWidth, b.wrapWidth);
	return false;
}

/**
 * Initialize a new instance of VuoImageTextData.
 */
VuoImageTextData VuoImageTextData_make()
{
	VuoImageTextData i = (VuoImageTextData) malloc(sizeof(struct _VuoImageTextData));
	i->lineCounts = NULL;
	i->lineBounds = NULL;
	i->lineWidthsExcludingTrailingWhitespace = NULL;
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
	if(value->lineWidthsExcludingTrailingWhitespace) free(value->lineWidthsExcludingTrailingWhitespace);
	if(value->charAdvance) free(value->charAdvance);
	free(value);
}

typedef std::vector<VuoPoint2d> VuoCorners; ///< 4 corners of a (non-axis-aligned) rectangle.
typedef std::pair<std::string, VuoFontClass> VuoImageTextCacheDescriptor;  ///< Text, font.
typedef std::pair<std::pair<VuoImage, VuoCorners>, double> VuoImageTextCacheEntry;  ///< ((An image, its bounds), and the last time it was used)
typedef std::map<VuoImageTextCacheDescriptor, VuoImageTextCacheEntry> VuoImageTextCacheType;	///< A pool of images.
static VuoImageTextCacheType *VuoImageTextCache;	///< A pool of images.
static dispatch_semaphore_t VuoImageTextCache_semaphore;	///< Serializes access to VuoImageTextCache.
static dispatch_semaphore_t VuoImageTextCache_canceledAndCompleted;     ///< Signals when the last VuoImageTextCache cleanup has completed.
static volatile dispatch_source_t VuoImageTextCache_timer = NULL;  ///< Periodically cleans up VuoImageTextCache.
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
				imagesToRelease.push_back(item->second.first.first);
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
 * Ensure VuoImageTextCache_timer starts out null,
 * since its value is used in `VuoImageTextCache_fini`.
 * (For example, when a text-rendering composition
 * is exported as an FFGL plugin and used in VDMX,
 * the static initializer above doesn't happen,
 * so VDMX crashes on quit.)
 */
extern "C" void __attribute__((constructor)) VuoImageTextCache_preinit(void)
{
	VuoImageTextCache_timer = nullptr;
}

/**
 * Initializes the cache.
 */
static void VuoImageTextCache_init(void)
{
	VuoImageTextCache_semaphore = dispatch_semaphore_create(1);
	VuoImageTextCache_canceledAndCompleted = dispatch_semaphore_create(0);
	VuoImageTextCache = new VuoImageTextCacheType;

	VuoImageTextCache_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
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
		VuoRelease(item->second.first.first);
	}

	delete VuoImageTextCache;
	dispatch_release(VuoImageTextCache_timer);
	VuoImageTextCache_timer = NULL;
}

/**
 * Get the vertical scaling used for text.
 * @version200New
 */
VuoReal VuoImageText_getVerticalScale(VuoReal screenWidth, VuoReal backingScaleFactor)
{
	return (screenWidth / backingScaleFactor) / VuoGraphicsWindowDefaultWidth;
}

/**
 * Formats the specified `text` for rendering using `font` and `backingScaleFactor`.
 */
static CFArrayRef VuoImageText_createCTLines(
	VuoText text,
	VuoFont font,
	VuoReal backingScaleFactor,
	VuoReal verticalScale,
	VuoReal rotation,
	VuoReal wrapWidth,
	bool includeTrailingWhiteSpace,
	CTFontRef *ctFont,
	CGColorRef *cgColor,
	CGColorSpaceRef *colorspace,
	VuoImageTextData textImageData,
	CGAffineTransform *outTransform)
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

	*outTransform = CGAffineTransformScale(CGAffineTransformMakeRotation(-rotation), 1., verticalScale);

	// Create a rectangle to optionally limit the text width.
	double wrapWidthPixels = fmax(1, wrapWidth * VuoGraphicsWindowDefaultWidth/2. * backingScaleFactor);
	CGMutablePathRef path = CGPathCreateMutable();
	CGPathAddRect(path, NULL, CGRectMake(0, 0, wrapWidthPixels, INFINITY));

	// Split the user's text into lines, both on manually-added linebreaks and automatically word-wrapped at `wrapWidth`.
	CFStringRef cfText = CFStringCreateWithCStringNoCopy(NULL, text, kCFStringEncodingUTF8, kCFAllocatorNull);
	CFIndex characterCount = CFStringGetLength(cfText);
	CFStringRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName, kCTUnderlineStyleAttributeName, kCTKernAttributeName };
	CFTypeRef values[] = { *ctFont, *cgColor, underlineNumber, kernNumber };
	CFDictionaryRef attr = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values, sizeof(keys) / sizeof(keys[0]), NULL, NULL);
	CFAttributedStringRef attrString = CFAttributedStringCreate(NULL, cfText, attr);
	CTFramesetterRef framesetter = CTFramesetterCreateWithAttributedString(attrString);
	CTFrameRef frame = CTFramesetterCreateFrame(framesetter, CFRangeMake(0,0), path, NULL);
	CFArrayRef ctLines = CTFrameGetLines(frame);
	CFRetain(ctLines);
	CFIndex lineCount = CFArrayGetCount(ctLines);
	if (text[strlen(text) - 1] == '\n')
		++lineCount;

	// Get the bounds of each line of text, and union them into bounds for the entire block of text.
	double ascent = CTFontGetAscent(*ctFont);
	double descent = CTFontGetDescent(*ctFont);
	double leading = CTFontGetLeading(*ctFont);

	double lineHeight = ascent + descent + leading;
	CGRect bounds = CGRectMake(0, 0, 0, 0);
	CGRect* lineBounds = (CGRect *)malloc(sizeof(CGRect) * lineCount);
	unsigned int* lineCounts = (unsigned int*) malloc(sizeof(unsigned int) * lineCount);
	textImageData->lineWidthsExcludingTrailingWhitespace = (VuoReal *)malloc(sizeof(VuoReal) * lineCount);
	textImageData->lineXOrigins = (VuoReal *)malloc(sizeof(VuoReal) * lineCount);
	VuoReal* charAdvance = (VuoReal*) calloc(characterCount, sizeof(VuoReal));

	for (CFIndex i = 0; i < lineCount; ++i)
	{
		// Run through the loop body for the empty trailing newline (if any), adding normal lineheight with zero width.
		CTLineRef ctLine = nullptr;
		if (i < CFArrayGetCount(ctLines))
			ctLine = (CTLineRef) CFArrayGetValueAtIndex(ctLines, i);

		CFRange stringRange = ctLine ? CTLineGetStringRange(ctLine) : CFRangeMake(0,0);
		lineCounts[i] = stringRange.length;

		// get each individual character offset
		CGFloat secondaryOffset;
		CGFloat previousOffset = ctLine ? CTLineGetOffsetForStringIndex(ctLine, stringRange.location, &secondaryOffset ) : 0;

		for(CFIndex index = stringRange.location; index < stringRange.location + stringRange.length; index++)
		{
			CGFloat offset = CTLineGetOffsetForStringIndex(ctLine, MIN(stringRange.location + stringRange.length, index + 1), &secondaryOffset );
			charAdvance[index] = (VuoReal) (offset - previousOffset);
			previousOffset = offset;
		}

		CGRect lineImageBounds = ctLine ? CTLineGetImageBounds(ctLine, cgContext) : CGRectZero;
		double width = CGRectGetWidth(lineImageBounds);
		textImageData->lineWidthsExcludingTrailingWhitespace[i] = lineImageBounds.size.width;
		textImageData->lineXOrigins[i] = lineImageBounds.origin.x;
		if (includeTrailingWhiteSpace && ctLine)
			width += CTLineGetTrailingWhitespaceWidth(ctLine);
		lineBounds[i] = CGRectMake(CGRectGetMinX(lineImageBounds), lineHeight * i - ascent, width, lineHeight);

		// Can't use CGRectUnion since it shifts the origin to (0,0), cutting off the glyph's ascent and strokes left of the origin (e.g., Zapfino's "g").
		if (CGRectGetMinX(lineBounds[i]) < CGRectGetMinX(bounds))
			bounds.origin.x = CGRectGetMinX(lineBounds[i]);
		if (CGRectGetMinY(lineBounds[i]) < CGRectGetMinY(bounds))
			bounds.origin.y = CGRectGetMinY(lineBounds[i]);
		if (CGRectGetMaxX(lineBounds[i]) > CGRectGetMaxX(bounds))
			bounds.size.width += CGRectGetMaxX(lineBounds[i]) - CGRectGetMaxX(bounds);

		// Final bounds should always include the full first line's height.
		if (i == 0)
			bounds.size.height += lineHeight;
		else
			bounds.size.height += lineHeight * font.lineSpacing;
	}

	// The 2 extra pixels are to account for the antialiasing on strokes that touch the edge of the glyph bounds — without those pixels, some edge strokes are slightly cut off.
	const unsigned int AA_STROKE_PAD = 2;

	CGRect transformedBounds = CGRectApplyAffineTransform(bounds, *outTransform);
	CGPoint transformedTopLeft = CGPointApplyAffineTransform(bounds.origin, *outTransform);
	CGPoint transformedTopRight = CGPointApplyAffineTransform(CGPointMake(bounds.origin.x+bounds.size.width, bounds.origin.y), *outTransform);
	CGPoint transformedBottomRight = CGPointApplyAffineTransform(CGPointMake(bounds.origin.x+bounds.size.width, bounds.origin.y+bounds.size.height), *outTransform);
	CGPoint transformedBottomLeft = CGPointApplyAffineTransform(CGPointMake(bounds.origin.x, bounds.origin.y+bounds.size.height), *outTransform);

	unsigned int width = ceil(CGRectGetWidth(transformedBounds)) + AA_STROKE_PAD;
	unsigned int height = ceil(CGRectGetHeight(transformedBounds)) + AA_STROKE_PAD;

	textImageData->width = width;
	textImageData->height = height;
	textImageData->lineHeight = lineHeight;
	textImageData->bounds = VuoRectangle_make(CGRectGetMidX(bounds), CGRectGetMidY(bounds), CGRectGetWidth(bounds), CGRectGetHeight(bounds));
	textImageData->transformedBounds = VuoRectangle_make(CGRectGetMidX(transformedBounds), CGRectGetMidY(transformedBounds), CGRectGetWidth(transformedBounds), CGRectGetHeight(transformedBounds));
	textImageData->transformedCorners[0] = (VuoPoint2d){(float)(transformedTopLeft.x - transformedBounds.origin.x), (float)(transformedTopLeft.y - transformedBounds.origin.y)};
	textImageData->transformedCorners[1] = (VuoPoint2d){(float)(transformedTopRight.x - transformedBounds.origin.x), (float)(transformedTopRight.y - transformedBounds.origin.y)};
	textImageData->transformedCorners[2] = (VuoPoint2d){(float)(transformedBottomRight.x - transformedBounds.origin.x), (float)(transformedBottomRight.y - transformedBounds.origin.y)};
	textImageData->transformedCorners[3] = (VuoPoint2d){(float)(transformedBottomLeft.x - transformedBounds.origin.x), (float)(transformedBottomLeft.y - transformedBounds.origin.y)};
	textImageData->lineCount = lineCount;
	textImageData->lineCounts = lineCounts;
	textImageData->lineBounds = (VuoRectangle*) malloc(sizeof(VuoRectangle) * lineCount);
	textImageData->charAdvance = charAdvance;
	textImageData->charCount = characterCount;
	textImageData->horizontalAlignment = font.alignment;

	for (int i = 0; i < lineCount; i++)
	{
		CGRect bb = CGRectApplyAffineTransform(lineBounds[i], *outTransform);
		textImageData->lineBounds[i] = VuoRectangle_make(CGRectGetMidX(bb), CGRectGetMidY(bb), CGRectGetWidth(bb), CGRectGetHeight(bb));
	}

	free(lineBounds);

	// Release the temporary context.
	CGContextRelease(cgContext);

	CFRelease(attr);
	CFRelease(attrString);
	CFRelease(framesetter);
	CFRelease(path);
	CFRelease(frame);
	CFRelease(cfText);
	CFRelease(kernNumber);
	CFRelease(underlineNumber);

	return ctLines;
}

/**
 * Returns a rectangle (in points) that fully encloses the specified `text` when rendered with the specified `font`.
 *
 * Depending on the font's design (e.g., if the font is monospace), the rectangle may be larger than the actual glyphs.
 *
 * @version200Changed{Added `backingScaleFactor`, `verticalScale`, `rotation`, `wrapWidth` arguments.}
 */
VuoRectangle VuoImage_getTextRectangle(VuoText text, VuoFont font, VuoReal backingScaleFactor, VuoReal verticalScale, VuoReal rotation, float wrapWidth, bool includeTrailingWhiteSpace)
{
	VuoPoint2d corners[4];
	// This should be faster than VuoImage_getTextImageData(), since we should be hitting VuoImage_makeText()'s cache.
	VuoImage_makeText(text, font, backingScaleFactor, verticalScale, rotation, wrapWidth, corners);

	double minX = MIN(corners[0].x, MIN(corners[1].x, MIN(corners[2].x, corners[3].x)));
	double minY = MIN(corners[0].y, MIN(corners[1].y, MIN(corners[2].y, corners[3].y)));
	double maxX = MAX(corners[0].x, MAX(corners[1].x, MAX(corners[2].x, corners[3].x)));
	double maxY = MAX(corners[0].y, MAX(corners[1].y, MAX(corners[2].y, corners[3].y)));

	return VuoRectangle_make(0, 0, maxX-minX, maxY-minY);
}

/**
 * Returns a struct containing all information necessary to calculate text and character size and placement when rendered.
 *
 * If text is null or empty, null is returned.
 *
 * @version200Changed{Added `backingScaleFactor`, `verticalScale`, `rotation` arguments.}
 */
VuoImageTextData VuoImage_getTextImageData(VuoText text, VuoFont font, VuoReal backingScaleFactor, VuoReal verticalScale, VuoReal rotation, bool includeTrailingWhiteSpace)
{
	if (!VuoText_length(text))
		return NULL;

	CTFontRef ctFont;
	CGColorRef cgColor;
	CGColorSpaceRef colorspace;
	VuoImageTextData textData = VuoImageTextData_make();
	CGAffineTransform transform;
	CFArrayRef ctLines = VuoImageText_createCTLines(text, font, backingScaleFactor, verticalScale, rotation, INFINITY, includeTrailingWhiteSpace, &ctFont, &cgColor, &colorspace, textData, &transform);
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
	VuoRectangle textBounds = VuoImage_getTextRectangle(text, font, backingScaleFactor, 1, 0, INFINITY, includeTrailingWhiteSpace);

	double s = VuoImageText_getVerticalScale(windowSize.x, backingScaleFactor);
	double w = textBounds.size.x * s;
	double h = textBounds.size.y * s;

	VuoPoint2d size;
	size.x = (w / windowSize.x) * 2;
	size.y = size.x * (h / w);

	return size;
}

#define ScreenToGL(x) (((x) / screenWidthInPixels) * 2.)	///< Shortcut macro for converting to GL coordinates in VuoImageTextData_convertToVuoCoordinates function.

 /**
 * Get the line height of a font in Vuo coordinates. Useful in cases where no text data is available (ie, you should prefer using
 * textData->lineHeight where available).
 * @version200New
 */
VuoReal VuoImageText_getLineHeight(VuoFont font, VuoReal screenWidthInPixels, VuoReal backingScaleFactor)
{
	CFStringRef fontNameCF = NULL;

	if (font.fontName)
		fontNameCF = CFStringCreateWithCString(NULL, font.fontName, kCFStringEncodingUTF8);

	CTFontRef ctFont = CTFontCreateWithName(fontNameCF ? fontNameCF : CFSTR(""), font.pointSize * backingScaleFactor, NULL);

	if (fontNameCF)
		CFRelease(fontNameCF);

	double ascent = CTFontGetAscent(ctFont);
	double descent = CTFontGetDescent(ctFont);
	double leading = CTFontGetLeading(ctFont);
	VuoReal lineHeight = ascent + descent + leading;

	CFRelease(ctFont);

	double scale = VuoImageText_getVerticalScale(screenWidthInPixels, backingScaleFactor);
	return ScreenToGL(lineHeight * scale);
}

/**
 * Convert VuoImageTextData from pixel coordinates to Vuo coordinates in place.
 */
void VuoImageTextData_convertToVuoCoordinates(VuoImageTextData textData, VuoReal screenWidthInPixels, VuoReal backingScaleFactor)
{
	// @todo convert the other values too

	double scale = VuoImageText_getVerticalScale(screenWidthInPixels, backingScaleFactor);
	double w = textData->width * scale;
	double h = textData->height * scale;

	textData->width = (w / screenWidthInPixels) * 2;
	textData->height = textData->width * (h / w);

	textData->lineHeight = ScreenToGL(textData->lineHeight * scale);

	textData->bounds.center = VuoPoint2d_make(ScreenToGL(textData->bounds.center.x * scale), ScreenToGL(textData->bounds.center.y * scale));
	textData->bounds.size  = VuoPoint2d_make(ScreenToGL(textData->bounds.size.x * scale), ScreenToGL(textData->bounds.size.y * scale));

	for (int i = 0; i < textData->lineCount; i++)
	{
		textData->lineBounds[i].center = VuoPoint2d_make(ScreenToGL(textData->lineBounds[i].center.x * scale), ScreenToGL(textData->lineBounds[i].center.y * scale));
		textData->lineBounds[i].size = VuoPoint2d_make(ScreenToGL(textData->lineBounds[i].size.x * scale), ScreenToGL(textData->lineBounds[i].size.y * scale));
	}

	for (int n = 0; n < textData->charCount; n++)
	{
		float advance = textData->charAdvance[n] * scale;
		textData->charAdvance[n] = ScreenToGL(advance);
	}
}

#undef ScreenToGL

/**
 * Get the origin point of a line of text (bottom left point of first character billboard in line).
 * @version200New
 */
VuoPoint2d VuoImageTextData_getPositionForLineIndex(VuoImageTextData textData, unsigned int lineIndex)
{
	double x = 0;

	// if horizontal alignment is `left` the origin is just image bounds left.
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
 * Find the line index that the character at charIndex resides in. If lineStartCharIndex is not null, the character index of the first
 * character on the found line is stored.
 * In the event that charIndex is out of the bounds of textData, 0 or lineCounts - 1 is returned.
 *
 * @version200Changed{Added `lineIndex` argument.}
 */
unsigned int VuoImageTextData_getLineWithCharIndex(VuoImageTextData textData, unsigned int charIndex, unsigned int* lineStartCharIndex)
{
	// figure out which row of text the char index is on
	unsigned int lineIndex = 0;
	// the char index that the starting line begins with
	unsigned int lineStart = 0;

	while (lineIndex < textData->lineCount && charIndex >= lineStart + textData->lineCounts[lineIndex])
	{
		lineStart += textData->lineCounts[lineIndex];
		lineIndex++;
	}

	if(lineIndex >= textData->lineCount)
	{
		lineIndex = textData->lineCount - 1;
		lineStart -= textData->lineCounts[lineIndex];
	}

	if(lineStartCharIndex != NULL)
		*lineStartCharIndex = lineStart;

	return lineIndex;
}

/**
 * Get the starting point for a character index (bottom left corner).
 */
VuoPoint2d VuoImageTextData_getPositionForCharIndex(VuoImageTextData textData, unsigned int charIndex, unsigned int* lineIndex)
{
	charIndex = MAX(0, MIN(textData->charCount, charIndex));

	// figure out which row of text the char index is on
	unsigned int lineStart = 0;
	unsigned int lineIdx = VuoImageTextData_getLineWithCharIndex(textData, charIndex, &lineStart);
	VuoPoint2d pos = VuoImageTextData_getPositionForLineIndex(textData, lineIdx);

	// now that origin x and y are set, step through line to actual index
	for(int n = lineStart; n < charIndex; n++)
		pos.x += textData->charAdvance[n];

	if( lineIndex != NULL )
		*lineIndex = lineIdx;

	return pos;
}

/**
 * Given a start index and length of selected characters return an array of VuoRectangles encompassing the selection.
 * @version200New
 */
VuoRectangle* VuoImageTextData_getRectsForHighlight(VuoImageTextData textData, unsigned int selectionStartIndex, unsigned int selectionLength, unsigned int* lineCount)
{
	std::vector<VuoRectangle> rects;

	unsigned int lineStart = 0;
	unsigned int lineIndex = VuoImageTextData_getLineWithCharIndex(textData, selectionStartIndex, &lineStart);
	VuoPoint2d origin = VuoImageTextData_getPositionForLineIndex(textData, lineIndex);

	// now that origin x and y are set for the starting line, step through line to actual character start index
	for(int n = lineStart; n < selectionStartIndex; n++)
		origin.x += textData->charAdvance[n];

	float width = 0;

	unsigned int curIndex = selectionStartIndex;
	unsigned int textLength = textData->charCount;

	while(curIndex < selectionStartIndex + selectionLength)
	{
		width += curIndex < textLength ? textData->charAdvance[curIndex] : 0;
		curIndex++;

		if( curIndex >= lineStart + textData->lineCounts[lineIndex] || curIndex >= selectionStartIndex + selectionLength )
		{
			float w = fmax(.01, width);
			float h = textData->lineHeight;
			rects.push_back( VuoRectangle_make(origin.x + (w * .5), origin.y + (h * .5), w, h) );

			lineStart += textData->lineCounts[lineIndex];
			lineIndex++;

			if(lineIndex >= textData->lineCount)
				break;

			origin = VuoImageTextData_getPositionForLineIndex(textData, lineIndex);
			width = 0;
		}
	}

	*lineCount = rects.size();
	VuoRectangle* copy = (VuoRectangle*) malloc(sizeof(VuoRectangle) * (*lineCount));
	std::copy(rects.begin(), rects.end(), copy);
	return copy;
}

/**
 * Get the index of the first character at line index.
 * @version200New
 */
unsigned int VuoImageTextData_getCharIndexForLine(VuoImageTextData textData, unsigned int lineIndex)
{
	unsigned int charIndex = 0;
	unsigned int index = MIN(textData->lineCount, lineIndex);
	for(unsigned int i = 0; i < index; i++)
		charIndex += textData->lineCounts[i];
	return charIndex;
}

/**
 * Get a rect that contains a row of text starting at index.  The number of characters from index to line end is stored in length.
 * @version200New
 */
VuoRectangle VuoImageTextData_layoutRowAtIndex(VuoImageTextData textData, unsigned int index, unsigned int* charactersRemaining)
{
	unsigned int lineIndex;
	VuoPoint2d begin = VuoImageTextData_getPositionForCharIndex(textData, index, &lineIndex);
	unsigned int lineBegin = VuoImageTextData_getCharIndexForLine(textData, lineIndex);
	*charactersRemaining = (textData->lineCounts[lineIndex] - (index - lineBegin));
	float width = textData->lineBounds[lineIndex].size.x - begin.x;
	return VuoRectangle_makeTopLeft(begin.x, textData->lineHeight, width, textData->lineHeight);
}

/**
 * Return the character index (0 indexed) nearest to point, where point is relative to the pivot point of a text layer.
 */
int VuoImageTextData_getNearestCharToPoint(VuoImageTextData textData, VuoPoint2d point)
{
	int index = 0;

	// point is already in model space, but it needs to be relative to image where (0,0) is the
	// center of the image.  if the text billboard is not centered we need to translate the point
	// to suit.
	VuoHorizontalAlignment h = VuoAnchor_getHorizontal(textData->billboardAnchor);
	VuoVerticalAlignment v = VuoAnchor_getVertical(textData->billboardAnchor);

	if(h == VuoHorizontalAlignment_Left)
		point.x -= textData->width * .5;
	else if(h == VuoHorizontalAlignment_Right)
		point.x += textData->width * .5;

	if(v == VuoVerticalAlignment_Bottom)
		point.y -= textData->height * .5;
	else if(v == VuoVerticalAlignment_Top)
		point.y += textData->height * .5;

	for(int r = 0; r < textData->lineCount; r++)
	{
		bool lastLine = r == textData->lineCount - 1;
		VuoRectangle lineBounds = textData->lineBounds[r];
		VuoPoint2d lineOrigin = VuoImageTextData_getPositionForLineIndex(textData, r);

		// if the point is above this line, or it's the last line, that means
		// this row is the nearest to the cursor
		if(point.y > lineOrigin.y || lastLine)
		{
			// left of the start of this line, so first index it is
			if(point.x < lineOrigin.x)
				return index;

			// right of the end of this line, so last index (before new line char) unless it's the last line
			else if(point.x > (lineOrigin.x + lineBounds.size.x))
				return index + textData->lineCounts[r] - (lastLine ? 0 : 1);

			for(int i = 0; i < textData->lineCounts[r]; i++)
			{
				float advance = textData->charAdvance[index + i];

				if(point.x < lineOrigin.x + advance * .5)
					return index + i;

				lineOrigin.x += advance;
			}

			return index + textData->lineCounts[r] + (lastLine ? 1 : 0);
		}

		index += textData->lineCounts[r];
	}

	// never should get here, but just in case
	return textData->charCount;
}

/**
 * Creates an image containing the specified text.
 *
 * If `outCorners` is not null, this function fills it with the text's 4 transformed bounds in pixels relative to the top-left of the image.
 *
 * @version200Changed{Added `verticalScale`, `rotation`, `wrapwidth`, `outCorners` arguments.}
 */
VuoImage VuoImage_makeText(VuoText text, VuoFont font, float backingScaleFactor, float verticalScale, float rotation, float wrapWidth, VuoPoint2d *outCorners)
{
	if (VuoText_isEmpty(text))
		return NULL;

	if (font.pointSize < 0.00001
	  || verticalScale < 0.00001)
		return NULL;

	// Is there an image ready in the cache?
	static dispatch_once_t initCache = 0;
	dispatch_once(&initCache, ^ {
		VuoImageTextCache_init();
	});
	VuoFontClass fc(font, backingScaleFactor, verticalScale, rotation, wrapWidth);
	VuoImageTextCacheDescriptor descriptor(text, fc);
	dispatch_semaphore_wait(VuoImageTextCache_semaphore, DISPATCH_TIME_FOREVER);
	{
		VuoImageTextCacheType::iterator e = VuoImageTextCache->find(descriptor);
		if (e != VuoImageTextCache->end())
		{
			// VLog("found in cache");
			VuoImage image = e->second.first.first;
			if (outCorners)
				memcpy(outCorners, &e->second.first.second[0], sizeof(VuoPoint2d)*4);
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
	CGAffineTransform transform;
	CFArrayRef ctLines = VuoImageText_createCTLines(text, font, backingScaleFactor, verticalScale, rotation, wrapWidth, false, &ctFont, &cgColor, &colorspace, textData, &transform);


	// Create the rendering context.
	// VuoImage_makeFromBuffer() expects a premultiplied buffer.
	CGContextRef cgContext = CGBitmapContextCreate(NULL, textData->width, textData->height, 8, textData->width * 4, colorspace, kCGImageAlphaPremultipliedLast);

	// Draw a red box at the border of the entire image.
	// CGContextSetRGBStrokeColor(cgContext, 1.0, 0.0, 0.0, 1.0); /// nocommit
	// CGContextStrokeRect(cgContext, CGRectMake(0,0,textData->width,textData->height)); /// nocommit

	// Antialiasing is enabled by default.
	// Disable it to test sharpness.
//	CGContextSetShouldAntialias(cgContext, false);
//	CGContextSetAllowsAntialiasing(cgContext, false);
//	CGContextSetShouldSmoothFonts(cgContext, false);
//	CGContextSetAllowsFontSmoothing(cgContext, false);

	// Subpixel positioning is enabled by default.
	// Disabling it makes some letters sharper,
	// but makes spacing less consistent.
//	CGContextSetShouldSubpixelPositionFonts(cgContext, false);
//	CGContextSetAllowsFontSubpixelPositioning(cgContext, false);
//	CGContextSetShouldSubpixelQuantizeFonts(cgContext, false);
//	CGContextSetAllowsFontSubpixelQuantization(cgContext, false);

	// Vertically flip, since VuoImage_makeFromBuffer() expects a flipped buffer.
	CGContextSetTextMatrix(cgContext, CGAffineTransformMakeScale(1.0, -1.0));

	if (outCorners)
		memcpy(outCorners, textData->transformedCorners, sizeof(VuoPoint2d)*4);

	// Move the origin so the entire transformed text rectangle is within the image area.
	CGContextTranslateCTM(cgContext, textData->transformedCorners[0].x, textData->transformedCorners[0].y);

	// Offset by AA_STROKE_PAD/2, depending on rotation, to keep the edges in bounds.
	CGContextTranslateCTM(cgContext, fabs(cos(rotation)), fabs(sin(rotation)));

	// Apply verticalScale and rotation.
	CGContextConcatCTM(cgContext, transform);

	VuoRectangle bounds = textData->bounds;

	// Draw each line of text.
	CFIndex lineCount = CFArrayGetCount(ctLines);
	for (CFIndex i = 0; i < lineCount; ++i)
	{
		CTLineRef ctLine = (CTLineRef)CFArrayGetValueAtIndex(ctLines, i);

		float textXPosition = -(bounds.center.x - (bounds.size.x * .5));
		if (font.alignment == VuoHorizontalAlignment_Center)
			textXPosition += (bounds.size.x - textData->lineWidthsExcludingTrailingWhitespace[i] - textData->lineXOrigins[i]) / 2.;
		else if (font.alignment == VuoHorizontalAlignment_Right)
			textXPosition += bounds.size.x - textData->lineWidthsExcludingTrailingWhitespace[i] - textData->lineXOrigins[i];

		float textYPosition = -(bounds.center.y - (bounds.size.y * .5));
		textYPosition += textData->lineHeight * i * font.lineSpacing;

		CGContextSetTextPosition(cgContext, textXPosition, textYPosition);
		CTLineDraw(ctLine, cgContext);


		// textData->lineBounds is already scaled/rotated, so temporarily un-apply that matrix.
//		CGContextSaveGState(cgContext);
//		CGContextConcatCTM(cgContext, CGAffineTransformInvert(transform));
//		CGContextSetRGBStrokeColor(cgContext, 0, 1, 0, 1); // NOCOMMIT
//		VuoRectangle r = textData->lineBounds[i]; // NOCOMMIT
//		CGContextStrokeRect(cgContext, CGRectMake(r.center.x - r.size.x/2, r.center.y + r.size.y/2, r.size.x, r.size.y)); // NOCOMMIT
//		CGContextRestoreGState(cgContext);

//		CGContextSetRGBStrokeColor(cgContext, 0, 0, 1, 1); // NOCOMMIT
//		CGContextStrokeRect(cgContext, CGRectMake(textXPosition-1,textYPosition-1,2,2)); // NOCOMMIT
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

			VuoImage cachedImage = it->second.first.first;
			if (outCorners)
				memcpy(outCorners, &it->second.first.second[0], sizeof(VuoPoint2d)*4);
			it->second.second = VuoLogGetTime();
			dispatch_semaphore_signal(VuoImageTextCache_semaphore);
			return cachedImage;
		}

		VuoRetain(image);
		VuoCorners c(4);
		memcpy(&c[0], textData->transformedCorners, sizeof(VuoPoint2d)*4);
		VuoImageTextCacheEntry e(std::make_pair(image, c), VuoLogGetTime());
		(*VuoImageTextCache)[descriptor] = e;
		// VLog("stored in cache");

		dispatch_semaphore_signal(VuoImageTextCache_semaphore);
	}


	return image;
}
