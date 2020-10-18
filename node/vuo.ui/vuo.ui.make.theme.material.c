/**
 * @file
 * vuo.ui.make.theme.material node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */


// https://material.io/components

#include "node.h"

#include <json-c/json.h>

#include "VuoUiTheme.h"
#include "VuoList_VuoUiTheme.h"

#include "material-colors.h"

VuoModuleMetadata({
					  "title" : "Make Material Theme",
					  "keywords" : [
						  "interface", "gui", "user interface", "interact",
						  "material design system", "palette",
						  "group",
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "AdvanceThroughSlideshow.vuo", "AdjustColorWithSlider.vuo" ]
					  }
				  });

static const char *materialColorNames[] = {
	"red",
	"pink",
	"purple",
	"deep-purple",
	"indigo",
	"blue",
	"light-blue",
	"cyan",
	"teal",
	"green",
	"light-green",
	"lime",
	"yellow",
	"amber",
	"orange",
	"deep-orange",
	"brown",
	"grey",
	"blue-grey",
};

static const char *materialColorVariantNames[] = {
	"50",
	"100",
	"200",
	"300",
	"400",
	"500",
	"600",
	"700",
	"800",
	"900",
};

void nodeEvent
(
	VuoInputData(VuoInteger, {"menuItems":[
		{"value": 0, "name":"Red"},
		{"value": 1, "name":"Pink"},
		{"value": 2, "name":"Purple"},
		{"value": 3, "name":"Deep Purple"},
		{"value": 4, "name":"Indigo"},
		{"value": 5, "name":"Blue"},
		{"value": 6, "name":"Light Blue"},
		{"value": 7, "name":"Cyan"},
		{"value": 8, "name":"Teal"},
		{"value": 9, "name":"Green"},
		{"value":10, "name":"Light Green"},
		{"value":11, "name":"Lime"},
		{"value":12, "name":"Yellow"},
		{"value":13, "name":"Amber"},
		{"value":14, "name":"Orange"},
		{"value":15, "name":"Deep Orange"},
		{"value":16, "name":"Brown"},
		{"value":17, "name":"Grey"},
		{"value":18, "name":"Blue Grey"},
	], "default":0}) color,

	VuoInputData(VuoInteger, {"menuItems":[
		 {"value":2, "name":"200"},
		 {"value":3, "name":"300"},
		 {"value":4, "name":"400"},
		 {"value":5, "name":"500"},
		 {"value":6, "name":"600"},
		 {"value":7, "name":"700"},
		 {"value":8, "name":"800"},
		 {"value":9, "name":"900"},
	], "default":6}) colorVariant,

	VuoInputData(VuoInteger, {"menuItems":[
		 {"value":0, "name":"Low"},
		 {"value":1, "name":"Mid"},
		 {"value":2, "name":"High"},
	], "default":2}) emphasis,

	VuoInputData(VuoFont, {"default":{"fontName":"Avenir-Medium","pointSize":14}}) labelFont,

	VuoOutputData(VuoUiTheme) theme
)
{
	VuoList_VuoUiTheme elements = VuoListCreate_VuoUiTheme();
	VuoLocal(elements);

	const char *colorName = materialColorNames[VuoInteger_clamp(color, 0, 18)];
	json_object *jcolors = json_tokener_parse(materialColors);
	VuoDefer(^{ json_object_put(jcolors); });

	VuoInteger variantC = VuoInteger_clamp(colorVariant, 2, 9);

	VuoColor color0, color1, color2, colorH0, colorH1;
	{
		json_object *jcolor;
		if (!json_object_object_get_ex(jcolors, colorName, &jcolor))
			return;

		json_object *o;
		if (!json_object_object_get_ex(jcolor, materialColorVariantNames[VuoInteger_clamp(variantC-10, 0, 9)], &o))
			return;
		colorH1 = VuoColor_makeFromJson(o);

		if (!json_object_object_get_ex(jcolor, materialColorVariantNames[VuoInteger_clamp(variantC-10, 0, 9) + 1], &o))
			return;
		colorH0 = VuoColor_makeFromJson(o);

		if (!json_object_object_get_ex(jcolor, materialColorVariantNames[variantC-2], &o))
			return;
		color2 = VuoColor_makeFromJson(o);

		if (!json_object_object_get_ex(jcolor, materialColorVariantNames[variantC-1], &o))
			return;
		color1 = VuoColor_makeFromJson(o);

		if (!json_object_object_get_ex(jcolor, materialColorVariantNames[variantC], &o))
			return;
		color0 = VuoColor_makeFromJson(o);
	}

	VuoColor grey0, grey2, grey3;
	{
		json_object *jcolor;
		if (!json_object_object_get_ex(jcolors, "grey", &jcolor))
			return;

		json_object *o;
		if (!json_object_object_get_ex(jcolor, materialColorVariantNames[variantC], &o))
			return;
		grey0 = VuoColor_makeFromJson(o);

		if (!json_object_object_get_ex(jcolor, materialColorVariantNames[variantC-2], &o))
			return;
		grey2 = VuoColor_makeFromJson(o);

		if (!json_object_object_get_ex(jcolor, materialColorVariantNames[VuoInteger_clamp(variantC-3, 0, 9)], &o))
			return;
		grey3 = VuoColor_makeFromJson(o);
	}

	VuoColor black = (VuoColor){0,0,0,1};
	VuoColor white = (VuoColor){1,1,1,1};
	VuoColor clear = (VuoColor){0,0,0,0};

	VuoColor labelColor = (VuoColor_getLightness(color0) > 0.6)
		? black
		: white;

	// https://material.io/components/buttons
	VuoListAppendValue_VuoUiTheme(elements, VuoUiTheme_makeButtonRounded(
		0.25,
		0.08,
		labelFont,
		VuoAnchor_makeCentered(),
		(VuoPoint2d){0.03,0},

		// Label:
		emphasis == 0 ? color0 : (emphasis == 1 ? color0 : labelColor),
		emphasis == 0 ? color0 : (emphasis == 1 ? color0 : labelColor),
		emphasis == 0 ? color0 : (emphasis == 1 ? color0 : labelColor),

		// Background:
		emphasis == 0 ? clear   : (emphasis == 1 ? white   : color0),
		emphasis == 0 ? colorH1 : (emphasis == 1 ? colorH1 : color1),
		emphasis == 0 ? colorH0 : (emphasis == 1 ? colorH0 : color2),

		// Border:
		emphasis == 0 ? clear : (emphasis == 1 ? grey2 : clear),
		emphasis == 0 ? clear : (emphasis == 1 ? grey2 : clear),
		emphasis == 0 ? clear : (emphasis == 1 ? grey2 : clear),
		0.002,
		0.25
		));

	// https://material.io/components/selection-controls#checkboxes
	VuoListAppendValue_VuoUiTheme(elements, VuoUiTheme_makeToggleRounded(
		labelFont,

		// Label:
		black,
		black,
		black,
		black,
		black,

		// Checkmark:
		white,
		white,
		white,

		// Checkmark border:
		clear,
		clear,
		clear,

		// Checkbox background:
		white,   // inactive
		grey3,   // hovered
		grey0,   // pressed
		color0,  // toggled
		color1,  // toggled+hovered

		// Checkbox border:
		grey0,   // inactive
		grey0,   // hovered
		grey0,   // pressed
		color0,  // toggled
		color0,  // toggled+hovered

		0.002,  // Checkbox border thickness
		0.25,   // Checkbox roundness

		0.02  // Margin between Checkbox and Label:
		));

	// https://material.io/components/sliders
	VuoListAppendValue_VuoUiTheme(elements, VuoUiTheme_makeSliderRounded(
		labelFont,

		// Label:
		black,
		black,

		// Handle:
		0.025,  // width
		0.025,  // height
		0.02,   // border thickness
		1,      // roundness

		color0,
		color0,
		color0,

		// Handle border:
		clear,
		(VuoColor){color0.r,color0.g,color0.b,0.1},
		(VuoColor){color0.r,color0.g,color0.b,0.3},

		// Track:
		0.005,  // depth
		0,      // border thickness
		1,      // roundness

		// Active Track:
		color0,
		color0,

		// Active Track Border:
		clear,
		clear,

		// Inactive Track:
		colorH0,
		colorH0,

		// Inactive Track Border:
		clear,
		clear,

		0.01  // Margin between Track and Label
		));

	// https://material.io/components/text-fields
	VuoListAppendValue_VuoUiTheme(elements, VuoUiTheme_makeTextFieldRounded(
		labelFont,
		VuoAnchor_make(VuoHorizontalAlignment_Left, VuoVerticalAlignment_Top),
		(VuoPoint2d){0.02,0.02},
		black,
		black,
		black,

		// Background:
		white,
		white,
		white,

		// Border:
		grey2,
		black,
		color0,
		0.002,   // border thickness

		color0,  // cursor
		colorH0, // selection

		0.25     // roundness
		));

	*theme = VuoUiTheme_makeGroup(elements);
}
