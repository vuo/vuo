/**
 * @file
 * vuo.ui.make.theme.text.rounded node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoUiTheme.h"
#include "VuoList_VuoUiTheme.h"

VuoModuleMetadata({
					  "title" : "Make Text Field Theme (Rounded)",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
 						  "isDeprecated": true, // Not ready for release yet. https://b33p.net/kosada/node/11630
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoFont, {"default":{
		"fontName":"HelveticaNeue-Light",
		"alignment":"left",
		"pointSize":28,
		"color":{"r":0, "g":0, "b":0, "a":1}
	}}) labelFont,
	VuoInputData(VuoFont, {"default":{
		"fontName":"HelveticaNeue-Light",
		"alignment":"left",
		"pointSize":28,
		"color":{"r":0, "g":0, "b":0, "a":0.25}
	}}) placeholderFont,
	VuoInputData(VuoAnchor, {"default":{"horizontalAlignment":"left", "verticalAlignment":"center"}}) labelAnchor,
	VuoInputData(VuoPoint2d, {
		"default":{"x":0.05,"y":0.05},
		"suggestedMin":{"x":0, "y":0},
		"suggestedMax":{"x":1, "y":1},
		"suggestedStep":{"x":0.01, "y":0.01}
	}) labelPadding,
	VuoInputData(VuoColor, {"default":{"r":0.95, "g":0.95, "b":0.95, "a":1.0}}) color,
	VuoInputData(VuoColor, {"default":{"r":0.41, "g":0.51, "b":0.61, "a":1.0}}) hoveredColor,
	VuoInputData(VuoColor, {"default":{"r":0.35, "g":0.45, "b":0.55, "a":1.0}}) pressedColor,
	VuoInputData(VuoColor, {"default":{"r":0.30, "g":0.40, "b":0.50, "a":1.0}}) borderColor,
	VuoInputData(VuoReal, {"default":0.15, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.01}) borderThickness,
	VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.01}) cornerRoundness,
	VuoOutputData(VuoUiTheme) theme
)
{
	*theme = VuoUiTheme_makeTextFieldRounded(
		labelFont,
		placeholderFont,
		labelAnchor,
		labelPadding,
		color,
		hoveredColor,
		pressedColor,
		borderColor,
		borderThickness,
		cornerRoundness);
}
