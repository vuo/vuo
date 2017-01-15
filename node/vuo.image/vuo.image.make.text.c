/**
 * @file
 * vuo.image.make.text node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageText.h"


VuoModuleMetadata({
					 "title" : "Make Text Image",
					 "keywords" : [ "font", "glyph", "line", "string", "typeface" ],
					 "version" : "1.0.2",
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
		VuoInputData(VuoFont, {"default":{"fontName":"HelveticaNeue-Light","pointSize":28}}) font,
		VuoOutputData(VuoImage) image
)
{
	*image = VuoImage_makeText(text, font, 1);
}
