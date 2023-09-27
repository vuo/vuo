/**
 * @file
 * VuoFont C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoFont_h
#define VuoFont_h

#ifdef __cplusplus
extern "C" {
#endif

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

#define VuoFont_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.

VuoFont VuoFont_make(VuoText fontName, VuoReal pointSize, VuoBoolean underline, VuoColor color, VuoHorizontalAlignment alignment, VuoReal characterSpacing, VuoReal lineSpacing);
VuoFont VuoFont_makeDefault(void);

VuoFont VuoFont_makeFromJson(struct json_object * js);
struct json_object * VuoFont_getJson(const VuoFont value);
char * VuoFont_getSummary(const VuoFont value);

bool VuoFont_areEqual(const VuoFont a, const VuoFont b);
bool VuoFont_isLessThan(const VuoFont a, const VuoFont b);

/// @{
/**
 * Automatically generated function.
 */
char * VuoFont_getString(const VuoFont value);
void VuoFont_retain(VuoFont value);
void VuoFont_release(VuoFont value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
