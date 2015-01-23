/**
 * @file
 * vuo.image.make.text node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>

#include "VuoFont.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <ApplicationServices/ApplicationServices.h>


VuoModuleMetadata({
					 "title" : "Make Text Image",
					 "keywords" : [ "font", "glyph", "line", "string" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "ApplicationServices.framework"
					 ],
					 "node": {
						 "exampleCompositions" : [ "RenderText.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"Hello World!"}) text,
		VuoInputData(VuoFont, {"default":{"fontName":"Helvetica","pointSize":28}}) font,
		VuoOutputData(VuoImage) image
)
{
	// Find a font matching VuoFont.
	CTFontRef ctFont;
	if (font.fontName)
	{
		CFStringRef fontName = CFStringCreateWithCString(NULL, font.fontName, kCFStringEncodingUTF8);
		ctFont = CTFontCreateWithName(fontName, font.pointSize, NULL);
		CFRelease(fontName);
	}
	else
		ctFont = CTFontCreateWithName(CFSTR(""), font.pointSize, NULL);

	CGColorRef cgColor = CGColorCreateGenericRGB(font.color.r, font.color.g, font.color.b, font.color.a);

	unsigned long underline = font.underline ? kCTUnderlineStyleSingle : kCTUnderlineStyleNone;
	CFNumberRef underlineNumber = CFNumberCreate(NULL, kCFNumberCFIndexType, &underline);

	float kern = (font.characterSpacing-1)*font.pointSize;
	CFNumberRef kernNumber = CFNumberCreate(NULL, kCFNumberFloatType, &kern);

	// Create a temporary context to get the bounds.
	CGColorSpaceRef colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	CGContextRef cgContext = CGBitmapContextCreate(NULL, 1, 1, 8, 4, colorspace, kCGImageAlphaPremultipliedLast);

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

		CFAttributedStringRef attributedLine;
		CFStringRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName, kCTUnderlineStyleAttributeName, kCTKernAttributeName };
		CFTypeRef values[] = { ctFont, cgColor, underlineNumber, kernNumber };
		CFDictionaryRef attr = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values, sizeof(keys) / sizeof(keys[0]), NULL, NULL);
		attributedLine = CFAttributedStringCreate(NULL, line, attr);
		CFRelease(attr);
		CFArrayAppendValue(attributedLines, attributedLine);

		CTLineRef ctLine = CTLineCreateWithAttributedString(attributedLine);
		CFArrayAppendValue(ctLines, ctLine);
	}

	// Get the bounds of each line of text, and union them into bounds for the entire block of text.
	double lineHeight=0;
	CGRect bounds = CGRectMake(0,0,0,0);
	CGRect lineBounds[lineCount];
	for (CFIndex i=0; i<lineCount; ++i)
	{
		CTLineRef ctLine = CFArrayGetValueAtIndex(ctLines, i);

		// For some fonts (such as Consolas), CTLineGetImageBounds doesn't return sufficient bounds --- the right side of the text gets cut off.
		// For other fonts (such as Zapfino), CTLineGetTypographicBounds doesn't return sufficient bounds --- the left side of its loopy "g" gets cut off.
		// So combine the results of both.
		double ascent, descent, leading;
		double width = CTLineGetTypographicBounds(ctLine, &ascent, &descent, &leading);
		CGRect lineImageBounds = CTLineGetImageBounds(ctLine, cgContext);
		width = fmax(width,lineImageBounds.size.width);
		width += CTLineGetTrailingWhitespaceWidth(ctLine);
		lineHeight = ascent+descent+leading;
		lineBounds[i] = CGRectMake(lineImageBounds.origin.x, lineHeight*i - ascent, width, lineHeight);

		// Can't use CGRectUnion since it shifts the origin to (0,0), cutting off the glyph's ascent and strokes left of the origin (e.g., Zapfino's "g").
		if (lineBounds[i].origin.x < bounds.origin.x)
			bounds.origin.x = lineBounds[i].origin.x;
		if (lineBounds[i].origin.y < bounds.origin.y)
			bounds.origin.y = lineBounds[i].origin.y;
		if (lineBounds[i].size.width > bounds.size.width)
			bounds.size.width = lineBounds[i].size.width;

		// Final bounds should always include the full first line's height.
		if (i==0)
			bounds.size.height += lineHeight;
		else
			bounds.size.height += lineHeight * font.lineSpacing;
	}
	unsigned int width = ceil(bounds.size.width)+2;
	unsigned int height = ceil(bounds.size.height)+2;

	// Release the temporary context.
	CGContextRelease(cgContext);

	// Create the rendering context.
	// VuoImage_makeFromBuffer() expects a premultiplied buffer.
	cgContext = CGBitmapContextCreate(NULL, width, height, 8, width*4, colorspace, kCGImageAlphaPremultipliedLast);

	// Vertically flip, since VuoImage_makeFromBuffer() expects a flipped buffer.
	CGContextSetTextMatrix(cgContext, CGAffineTransformMakeScale(1.0, -1.0));

	// Draw each line of text.
	for (CFIndex i=0; i<lineCount; ++i)
	{
		CTLineRef ctLine = CFArrayGetValueAtIndex(ctLines, i);

		float textXPosition = 1 - bounds.origin.x;
		if (font.alignment == VuoHorizontalAlignment_Center)
			textXPosition += (bounds.size.width - lineBounds[i].size.width)/2.;
		else if (font.alignment == VuoHorizontalAlignment_Right)
			textXPosition += bounds.size.width - lineBounds[i].size.width;

		float textYPosition = 1 - bounds.origin.y;
		textYPosition += lineHeight*i*font.lineSpacing;

		CGContextSetTextPosition(cgContext, textXPosition, textYPosition);
		CTLineDraw(ctLine, cgContext);
	}

	// Make a VuoImage from the CGContext.
	*image = VuoImage_makeFromBuffer(CGBitmapContextGetData(cgContext), GL_RGBA, width, height);

	CFRelease(ctLines);
	CFRelease(attributedLines);
	CFRelease(lines);
	CFRelease(cfText);
	CGContextRelease(cgContext);
	CGColorSpaceRelease(colorspace);
	CFRelease(kernNumber);
	CFRelease(underlineNumber);
	CGColorRelease(cgColor);
	CFRelease(ctFont);
}
