/**
 * @file
 * VuoFont implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"

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
 * Returns a new VuoFont using default values.
 *
 * @version200New
 */
VuoFont VuoFont_makeDefault(void)
{
	return (VuoFont) { NULL, 18, false, (VuoColor){1,1,1,1}, VuoHorizontalAlignment_Center, 1, 1 };
}

/**
 * @ingroup VuoFont
 * Decodes the JSON object @c js to create a new value.
 */
VuoFont VuoFont_makeFromJson(json_object * js)
{
	return (VuoFont){
		VuoJson_getObjectValue(VuoText,                js, "fontName",         NULL),
		VuoJson_getObjectValue(VuoReal,                js, "pointSize",        0),
		VuoJson_getObjectValue(VuoBoolean,             js, "underline",        false),
		VuoJson_getObjectValue(VuoColor,               js, "color",            (VuoColor){1,1,1,1}),
		VuoJson_getObjectValue(VuoHorizontalAlignment, js, "alignment",        VuoHorizontalAlignment_Left),
		VuoJson_getObjectValue(VuoReal,                js, "characterSpacing", 1),
		VuoJson_getObjectValue(VuoReal,                js, "lineSpacing",      1)
	};
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
	json_object_object_add(js, "alignment", VuoHorizontalAlignment_getJson(value.alignment));
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
	const char *fontName = value.fontName ? value.fontName : "No font";
	const char *underline = value.underline ? "<li>underlined</li>" : "";
	char *color = VuoColor_getShortSummary(value.color);
	char *alignment = VuoHorizontalAlignment_getSummary(value.alignment);

	char *summary = VuoText_format("%s %gpt<ul><li>color %s</li>%s<li>%s-aligned</li><li>character spacing: %g</li><li>line spacing: %g</li></ul>",
						  fontName, value.pointSize, color, underline, alignment, value.characterSpacing, value.lineSpacing);

	free(color);
	free(alignment);
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
	VuoType_returnInequality(VuoText,                a.fontName,         b.fontName);
	VuoType_returnInequality(VuoReal,                a.pointSize,        b.pointSize);
	VuoType_returnInequality(VuoBoolean,             a.underline,        b.underline);
	VuoType_returnInequality(VuoColor,               a.color,            b.color);
	VuoType_returnInequality(VuoHorizontalAlignment, a.alignment,        b.alignment);
	VuoType_returnInequality(VuoReal,                a.characterSpacing, b.characterSpacing);
	VuoType_returnInequality(VuoReal,                a.lineSpacing,      b.lineSpacing);
	return false;
}
