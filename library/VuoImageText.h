/**
 * @file
 * VuoImageText interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoImage.h"
#include "VuoFont.h"

/**
 * Placement info about text when rendered to an image.
 */
typedef struct _VuoImageTextData
{
	VuoReal width;				///< Bounding rect width, including padding for anti-aliasing.  This value can optionally contain the trailing white space (as specified by includeTrailingWhiteSpace).
	VuoReal height;				///< Bounding rect height, including padding for anti-aliasing.
	VuoReal lineHeight;			///< The line height for the font.
	VuoRectangle bounds;		///< The bounding box of all text
	unsigned int lineCount;		///< The number of lines text was split into for rendering.
	unsigned int* lineCounts;	///< The number of characters for each line (not including new line character - this means sum of lineCounts != characterCount).
	VuoRectangle* lineBounds;	///< Bounding rect of each line of text.
	VuoReal* charAdvance;		///< The x advance for each character in each line, taking kerning into account.
	unsigned int charCount;		///< The number of characters represented by this data.
	VuoHorizontalAlignment horizontalAlignment; ///< The horizontal alignment of this font.
} *VuoImageTextData;

VuoImageTextData VuoImageTextData_make();
void VuoImageTextData_free(void* data);
void VuoImageTextData_convertToVuoCoordinates(VuoImageTextData textData, VuoReal screenWidthInPixels, VuoReal backingScaleFactor);
VuoPoint2d VuoImageTextData_getPositionForCharIndex(VuoImageTextData textData, unsigned int charIndex);
int VuoImageTextData_getNearestCharToPoint(VuoImageTextData textData, VuoPoint2d point);

VuoImageTextData VuoImage_getTextImageData(VuoText text, VuoFont font, bool includeTrailingWhiteSpace);
VuoRectangle VuoImage_getTextRectangle(VuoText text, VuoFont font, bool includeTrailingWhiteSpace);
VuoPoint2d VuoImageText_getTextSize(VuoText text, VuoFont font, VuoPoint2d windowSize, VuoReal backingScaleFactor, bool includeTrailingWhiteSpace);
VuoImage VuoImage_makeText(VuoText text, VuoFont font, float backingScaleFactor);

#ifdef __cplusplus
}
#endif
