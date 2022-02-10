/**
 * @file
 * vuo.layer.make.text node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"
#include "VuoSceneText.h"

VuoModuleMetadata({
					 "title" : "Make Text Layer",
					 "keywords" : [ "font", "glyph", "line", "string", "typeface", "instructions" ],
					 "version" : "2.0.0",
					 "node": {
						 "exampleCompositions" : [ "RenderTextLayer.vuo", "ShowTextAnchor.vuo" ]
					 },
					 "dependencies": [ "VuoSceneText" ]
				 });

uint64_t nodeInstanceInit(void)
{
	return VuoSceneObject_getNextId();
}

void nodeInstanceEvent
(
		VuoInstanceData(uint64_t) id,
		VuoInputData(VuoText, {"default":"Hello World!"}) text,
		VuoInputData(VuoFont, {"default":{"fontName":"Avenir-Medium","pointSize":24}}) font,
		VuoInputData(VuoAnchor, {"default":{"horizontalAlignment":"center", "verticalAlignment":"center"}}) anchor,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}, "name":"Position"}) center,
		VuoInputData(VuoReal, {"suggestedMin":-180.0, "suggestedMax":180.0, "suggestedStep":15.0}) rotation,
		VuoInputData(VuoReal, {"default":1, "auto":infinity, "autoSupersedesDefault":true, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.1}) wrapWidth,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) opacity,
		VuoOutputData(VuoLayer) layer
)
{
	VuoFont f = font;
	f.color.a *= opacity;
	*layer = (VuoLayer)VuoSceneText_make(text, f, true, wrapWidth, anchor);
	VuoSceneObject_setTransform((VuoSceneObject)*layer, VuoTransform_makeEuler((VuoPoint3d){center.x, center.y, 0}, (VuoPoint3d){0, 0, rotation * M_PI/180}, (VuoPoint3d){1,1,1}));
	VuoLayer_setId(*layer, *id);
}
