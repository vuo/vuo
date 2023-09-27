/**
 * @file
 * vuo.image.make.text node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageText.h"


VuoModuleMetadata({
					 "title" : "Make Text Image",
					 "keywords" : [ "font", "glyph", "line", "string", "typeface" ],
					 "version" : "1.1.0",
					 "dependencies" : [
						 "VuoImageText"
					 ],
					 "node": {
						 "exampleCompositions" : [ "RenderTextImage.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"Hello World!"}) text,
		VuoInputData(VuoFont, {"default":{"fontName":"Avenir-Medium","pointSize":24}}) font,
		VuoInputData(VuoReal, {"suggestedMin":-180.0, "suggestedMax":180.0, "suggestedStep":15.0}) rotation,
		VuoInputData(VuoInteger, {"default":1024, "auto":0, "autoSupersedesDefault":true, "suggestedMin":32, "suggestedMax":2048, "suggestedStep":32}) wrapWidth,
		VuoOutputData(VuoImage) image
)
{
	*image = VuoImage_makeText(text, font, 1, 1, rotation * M_PI/180,
		wrapWidth <= 0 ? INFINITY : wrapWidth * 2./VuoGraphicsWindowDefaultWidth,
		NULL);
}
