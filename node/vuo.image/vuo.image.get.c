/**
 * @file
 * vuo.image.get node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageGet.h"

VuoModuleMetadata({
					 "title" : "Get Image",
					 "keywords" : [
						 "download", "open", "load", "import", "http", "url", "file",
						 "photo", "picture", "bitmap", "texture", "icon",
						 "png",
						 "jpeg", "jpg",
						 "gif",
						 "bmp",
						 "OpenEXR", "exr", "ilm",
						 "hdr", "high", "dynamic", "range",
						 "psd", "Photoshop",
						 "raw", "cr2", "dng", "dcr", "nef", "raf", "mos", "kdc",
						 "Targa", "tga"
					 ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoImageGet"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "DisplayImage.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) imageURL,
		VuoOutputData(VuoImage) image
)
{
	*image = VuoImage_get(imageURL);
}
