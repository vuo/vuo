/**
 * @file
 * VuoImageText interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoImage.h"
#include "VuoFont.h"
#include "VuoAnchor.h"

/**
 * Placement info about text when rendered to an image.
 *
 * @version200Changed{Added `transformedBounds`, `transformedCorners`, `lineWidthsExcludingTrailingWhitespace`, `lineXOrigins`, `billboardAnchor`.}
 */
typedef struct _VuoImageTextData
{
	VuoReal width;                    ///< Bounding rect width, including padding for anti-aliasing.  This value can optionally contain the trailing white space (as specified by includeTrailingWhiteSpace).
	VuoReal height;                   ///< Bounding rect height, including padding for anti-aliasing.
	VuoReal lineHeight;               ///< The line height for the font.
	VuoRectangle bounds;              ///< The bounding box of all text, in pre-transform coordinates.
	VuoRectangle transformedBounds;   ///< The axis-aligned bounding box of all text, after applying `verticalScale` and `rotation`.
	VuoPoint2d transformedCorners[4]; ///< The text's 4 transformed rectangular corners, in pixels.
	unsigned int lineCount;           ///< The number of lines text was split into for rendering.
	unsigned int* lineCounts;         ///< The number of characters (codepoints, not bytes) for each line.  Includes (if present in source text) trailing whitespace (regardless of the `includeTrailingWhiteSpace` argument to some functions below) and trailing newline.
	VuoRectangle* lineBounds;         ///< Bounding rect of each line of text.
	VuoReal *lineWidthsExcludingTrailingWhitespace; ///< Same as lineBounds.size.x, but without trailing whitespace or transformation.
	VuoReal *lineXOrigins;                          ///< Same as lineBounds.origin.x, but without the transformation.
	VuoReal* charAdvance;             ///< The x advance for each character in each line, taking kerning into account.
	unsigned int charCount;           ///< The number of characters represented by this data.
	VuoHorizontalAlignment horizontalAlignment; ///< The horizontal alignment of this font.

	VuoAnchor billboardAnchor;        ///< The anchor point of the billboard used to render this text.  This value is unique in that it is not set by VuoImage_getTextImageData, rather, it is assigned by the function responsible for the VuoImage_makeText call.
} *VuoImageTextData;

VuoImageTextData VuoImageTextData_make();
void VuoImageTextData_free(void* data);
void VuoImageTextData_convertToVuoCoordinates(VuoImageTextData textData, VuoReal screenWidthInPixels, VuoReal backingScaleFactor);
VuoPoint2d VuoImageTextData_getPositionForLineIndex(VuoImageTextData textData, unsigned int lineIndex);
VuoPoint2d VuoImageTextData_getPositionForCharIndex(VuoImageTextData textData, unsigned int charIndex, unsigned int* lineIndex);
int VuoImageTextData_getNearestCharToPoint(VuoImageTextData textData, VuoPoint2d point);
VuoRectangle* VuoImageTextData_getRectsForHighlight(VuoImageTextData textData, unsigned int selectionStartIndex, unsigned int selectionLength, unsigned int* lineCount);
VuoRectangle VuoImageTextData_layoutRowAtIndex(VuoImageTextData textData, unsigned int index, unsigned int* charactersRemaining);
unsigned int VuoImageTextData_getCharIndexForLine(VuoImageTextData textData, unsigned int lineIndex);
VuoReal VuoImageText_getLineHeight(VuoFont font, VuoReal screenWidthInPixels, VuoReal backingScaleFactor);
VuoReal VuoImageText_getVerticalScale(VuoReal screenWidth, VuoReal screenBackingScaleFactor);

VuoImageTextData VuoImage_getTextImageData(VuoText text, VuoFont font, VuoReal backingScaleFactor, VuoReal verticalScale, VuoReal rotation, bool includeTrailingWhiteSpace);
VuoRectangle VuoImage_getTextRectangle(VuoText text, VuoFont font, VuoReal backingScaleFactor, VuoReal verticalScale, VuoReal rotation, float wrapWidth, bool includeTrailingWhiteSpace);
VuoPoint2d VuoImageText_getTextSize(VuoText text, VuoFont font, VuoPoint2d windowSize, VuoReal backingScaleFactor, bool includeTrailingWhiteSpace);
VuoImage VuoImage_makeText(VuoText text, VuoFont font, float backingScaleFactor, float verticalScale, float rotation, float wrapWidth, VuoPoint2d *outCorners);

#ifdef __cplusplus
}
#endif
