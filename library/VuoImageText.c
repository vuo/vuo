/**
 * @file
 * VuoImageText implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

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


/**
 * Formats the specified `text` for rendering using `font` and `backingScaleFactor`.
 */
static CFArrayRef VuoImageText_createCTLines(VuoText text, VuoFont font, float backingScaleFactor, CTFontRef *ctFont, CGColorRef *cgColor, CGColorSpaceRef *colorspace, unsigned int *width, unsigned int *height, double *lineHeight, CGRect *bounds, CGRect **lineBounds)
{
	if (font.fontName)
	{
		CFStringRef fontName = CFStringCreateWithCString(NULL, font.fontName, kCFStringEncodingUTF8);
		*ctFont = CTFontCreateWithName(fontName, font.pointSize * backingScaleFactor, NULL);
		CFRelease(fontName);
	}
	else
		*ctFont = CTFontCreateWithName(CFSTR(""), font.pointSize * backingScaleFactor, NULL);

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
		CFStringRef line = CFArrayGetValueAtIndex(lines, i);

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
		CTLineRef ctLine = CFArrayGetValueAtIndex(ctLines, i);

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
	if (!VuoText_length(text))
		return NULL;

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
		CTLineRef ctLine = CFArrayGetValueAtIndex(ctLines, i);

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
	VuoImage image = VuoImage_makeFromBuffer(CGBitmapContextGetData(cgContext), GL_RGBA, width, height, VuoImageColorDepth_8);

	CFRelease(ctLines);
	CGContextRelease(cgContext);
	CGColorSpaceRelease(colorspace);
	CGColorRelease(cgColor);
	CFRelease(ctFont);
	free(lineBounds);

	return image;
}
