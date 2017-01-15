/**
 * @file
 * VuoFont C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOFONT_H
#define VUOFONT_H

#include "VuoBoolean.h"
#include "VuoColor.h"
#include "VuoHorizontalAlignment.h"
#include "VuoInteger.h"
#include "VuoReal.h"
#include "VuoText.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoFont VuoFont
 * A font description: family, style, point size, underline.
 *
 * @{
 */

/**
 * A font description: family, style, point size, underline.
 */
typedef struct
{
	VuoText fontName;	///< Unique font machine name.  Includes variants such as bold, italic, and oblique.
	VuoReal pointSize;
	VuoBoolean underline;
	VuoColor color;
	VuoHorizontalAlignment alignment;
	VuoReal characterSpacing;	///< A value of 1.0 is normal character spacing.  Must be >= 0.0.
	VuoReal lineSpacing;	///< A value of 1.0 is normal line spacing.  Must be >= 0.0.
} VuoFont;

VuoFont VuoFont_make(VuoText fontName, VuoReal pointSize, VuoBoolean underline, VuoColor color, VuoHorizontalAlignment alignment, VuoReal characterSpacing, VuoReal lineSpacing);

VuoFont VuoFont_makeFromJson(struct json_object * js);
struct json_object * VuoFont_getJson(const VuoFont value);
char * VuoFont_getSummary(const VuoFont value);

#define VuoFont_SUPPORTS_COMPARISON
bool VuoFont_areEqual(const VuoFont a, const VuoFont b);
bool VuoFont_isLessThan(const VuoFont a, const VuoFont b);

/// @{
/**
 * Automatically generated function.
 */
VuoFont VuoFont_makeFromString(const char *str);
char * VuoFont_getString(const VuoFont value);
void VuoFont_retain(VuoFont value);
void VuoFont_release(VuoFont value);
/// @}

/**
 * @}
 */

#endif
