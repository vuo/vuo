/**
 * @file
 * VuoFont implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoFont.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Font",
					 "description" : "A font description: family, style, point size, underline.",
					 "keywords" : [ "text", "style", "bold", "italic", "sans", "serif", "monospace", "kerning", "spacing", "double-space", "alignment", "left", "center", "right" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoBoolean",
						"VuoColor",
						"VuoHorizontalAlignment",
						"VuoInteger",
						"VuoReal",
						"VuoText"
					 ]
				 });
#endif
/// @}

/**
 * Returns a new VuoFont with the specified attributes.
 */
VuoFont VuoFont_make(VuoText fontName, VuoReal pointSize, VuoBoolean underline, VuoColor color, VuoHorizontalAlignment alignment, VuoReal characterSpacing, VuoReal lineSpacing)
{
	VuoFont f;
	f.fontName = fontName;
	f.pointSize = pointSize;
	f.underline = underline;
	f.color = color;
	f.alignment = alignment;
	f.characterSpacing = characterSpacing;
	f.lineSpacing = lineSpacing;
	return f;
}

/**
 * Returns a string key for the specified horizontal alignment.
 */
static const char *getStringForAlignment(VuoHorizontalAlignment a)
{
	switch (a)
	{
		case VuoHorizontalAlignment_Left:
			return "left";
		case VuoHorizontalAlignment_Center:
			return "center";
		case VuoHorizontalAlignment_Right:
			return "right";
	}
}

/**
 * Returns the horizontal alignment for the specified string key.
 */
static VuoHorizontalAlignment getAlignmentForString(const char *s)
{
	if (strcmp(s, getStringForAlignment(VuoHorizontalAlignment_Left))==0)
		return VuoHorizontalAlignment_Left;
	if (strcmp(s, getStringForAlignment(VuoHorizontalAlignment_Center))==0)
		return VuoHorizontalAlignment_Center;
	if (strcmp(s, getStringForAlignment(VuoHorizontalAlignment_Right))==0)
		return VuoHorizontalAlignment_Right;

	VUserLog("no alignment for [%s]",s);
	return VuoHorizontalAlignment_Left;
}

/**
 * @ingroup VuoFont
 * Decodes the JSON object @c js to create a new value.
 */
VuoFont VuoFont_makeFromJson(json_object * js)
{
	VuoFont font = {NULL, 0, false, VuoColor_makeWithRGBA(1,1,1,1), VuoHorizontalAlignment_Left, 1, 1};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "fontName", &o))
		font.fontName = VuoText_makeFromJson(o);

	if (json_object_object_get_ex(js, "pointSize", &o))
		font.pointSize = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "underline", &o))
		font.underline = VuoBoolean_makeFromJson(o);

	if (json_object_object_get_ex(js, "color", &o))
		font.color = VuoColor_makeFromJson(o);

	if (json_object_object_get_ex(js, "alignment", &o))
		font.alignment = getAlignmentForString(json_object_get_string(o));

	if (json_object_object_get_ex(js, "characterSpacing", &o))
		font.characterSpacing = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "lineSpacing", &o))
		font.lineSpacing = VuoReal_makeFromJson(o);

	return font;
}

/**
 * @ingroup VuoFont
 * Encodes @c value as a JSON object.
 */
json_object * VuoFont_getJson(const VuoFont value)
{
	json_object *js = json_object_new_object();
	json_object_object_add(js, "fontName", VuoText_getJson(value.fontName));
	json_object_object_add(js, "pointSize", VuoReal_getJson(value.pointSize));
	json_object_object_add(js, "underline", VuoBoolean_getJson(value.underline));
	json_object_object_add(js, "color", VuoColor_getJson(value.color));
	json_object_object_add(js, "alignment", json_object_new_string(getStringForAlignment(value.alignment)));
	json_object_object_add(js, "characterSpacing", VuoReal_getJson(value.characterSpacing));
	json_object_object_add(js, "lineSpacing", VuoReal_getJson(value.lineSpacing));
	return js;
}

/**
 * @ingroup VuoFont
 * Returns a compact string representation of @c value (comma-separated components).
 */
char * VuoFont_getSummary(const VuoFont value)
{
	const char *fontName = value.fontName ? value.fontName : "(no font)";
	const char *underline = value.underline ? "<li>underlined</li>" : "";
	char *color = VuoColor_getSummary(value.color);
	const char *alignment = getStringForAlignment(value.alignment);

	char *summary = VuoText_format("%s %gpt<ul><li>color: %s</li>%s<li>%s-aligned</li><li>character spacing: %g</li><li>line spacing: %g</li></ul>",
						  fontName, value.pointSize, color, underline, alignment, value.characterSpacing, value.lineSpacing);

	free(color);
	return summary;
}

/**
 * Returns true if both fonts have the same name, size, underline status, color, alignment, and character/line spacing.
 */
bool VuoFont_areEqual(const VuoFont value1, const VuoFont value2)
{
	return VuoText_areEqual(value1.fontName, value2.fontName)
		&& VuoReal_areEqual(value1.pointSize, value2.pointSize)
		&& (value1.underline == value2.underline)
		&& VuoColor_areEqual(value1.color, value2.color)
		&& (value1.alignment == value2.alignment)
		&& VuoReal_areEqual(value1.characterSpacing, value2.characterSpacing)
		&& VuoReal_areEqual(value1.lineSpacing, value2.lineSpacing);
}

/**
 * Returns true if a < b.
 */
bool VuoFont_isLessThan(const VuoFont a, const VuoFont b)
{
	if (a.fontName == NULL && b.fontName != NULL) return true;
	if (a.fontName != NULL && b.fontName == NULL) return false;

	if (a.fontName && b.fontName)
	{
		int fontNameComparison = strcmp(a.fontName, b.fontName);
		if (fontNameComparison < 0) return true;
		if (fontNameComparison > 0) return false;
	}

	if (a.pointSize < b.pointSize) return true;
	if (a.pointSize > b.pointSize) return false;

	if (a.underline < b.underline) return true;
	if (a.underline > b.underline) return false;

	if (VuoColor_isLessThan(a.color, b.color)) return true;
	if (VuoColor_isLessThan(b.color, a.color)) return false;

	if (a.alignment < b.alignment) return true;
	if (a.alignment > b.alignment) return false;

	if (a.characterSpacing < b.characterSpacing) return true;
	if (a.characterSpacing > b.characterSpacing) return false;

	if (a.lineSpacing < b.lineSpacing) return true;
//	if (a.lineSpacing > b.lineSpacing) return false;

	return false;
}
