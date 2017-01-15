/**
 * @file
 * vuo.layer.make.text node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Make Text Layer",
					 "keywords" : [ "font", "glyph", "line", "string", "typeface", "instructions" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "RenderTextLayer.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"Hello World!"}) text,
		VuoInputData(VuoFont, {"default":{"fontName":"HelveticaNeue-Light","pointSize":28}}) font,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoOutputData(VuoLayer) layer
)
{
	layer->sceneObject = VuoSceneObject_makeText(text, font);
	layer->sceneObject.transform = VuoTransform_makeEuler(VuoPoint3d_make(center.x, center.y, 0), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1));
}
