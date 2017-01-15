/**
 * @file
 * vuo.image.wrapMode node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageWrapMode.h"
#include "VuoGlContext.h"
#include "VuoGlPool.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Change Wrap Mode",
					 "keywords" : [ "mirror", "rotate", "clamp", "edge", "clip", "transparent", "tile" ],
					 "version" : "2.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoGlPool",
						 "OpenGL.framework"
					 ],
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoImageWrapMode, {"default":"repeat"}) wrapMode,
		VuoOutputData(VuoImage) outputImage
)
{
	if (!image)
		return;

	VuoImage img = VuoImage_makeCopy(image, false);
	VuoImage_setWrapMode(img, wrapMode);
	*outputImage = img;
}
