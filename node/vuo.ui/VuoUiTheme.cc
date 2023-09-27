/**
 * @file
 * VuoUiTheme implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "VuoUiTheme.h"
}

#include "VuoUiThemeBase.hh"

/// @{
#ifdef VUO_COMPILER
extern "C" {
VuoModuleMetadata({
					  "title" : "UI Theme",
					  "description" : "A visual style for UI widgets.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoRenderedLayers",
						  "VuoSerializable",
						  "VuoUiThemeGroup",
						  "VuoUiThemeButtonRounded",
						  "VuoUiThemeTextFieldRounded",
						  "VuoUiThemeToggleRounded",
						  "VuoUiThemeSliderRounded",
						  "VuoList_VuoUiTheme"
					  ]
				  });
}
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 */
VuoUiTheme VuoUiTheme_makeFromJson(json_object *js)
{
	return reinterpret_cast<VuoUiTheme>(VuoSerializable::makeFromJson(js));
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoUiTheme_getJson(const VuoUiTheme v)
{
	VuoSerializable *value = (VuoSerializable *)v;
	return value != NULL ? value->getJson() : NULL;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoUiTheme_getSummary(const VuoUiTheme v)
{
	VuoSerializable *value = (VuoSerializable *)v;
	return value != NULL ? value->getSummary() : strdup("No theme");
}

/**
 * Returns true if the two values are equal.
 */
bool VuoUiTheme_areEqual(const VuoUiTheme a, const VuoUiTheme b)
{
	if (!a || !b)
		return !a && !b;

	VuoSerializable *valueA = (VuoSerializable *)a;
	VuoSerializable *valueB = (VuoSerializable *)b;
	return *valueA == *valueB;
}

/**
 * Returns true if `a` sorts before `b`.
 */
bool VuoUiTheme_isLessThan(const VuoUiTheme a, const VuoUiTheme b)
{
	// Treat a null theme as greater than a non-null theme,
	// so the more useful non-null theme sorts to the beginning of the list.
	if (!a || !b)
		return a && !b;

	VuoSerializable *valueA = (VuoSerializable *)a;
	VuoSerializable *valueB = (VuoSerializable *)b;
	return *valueA < *valueB;
}
