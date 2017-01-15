/**
 * @file
 * vuo.image.fetch node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageGet.h"

VuoModuleMetadata({
					 "title" : "Fetch Image",
					 "keywords" : [
						 "download", "open", "load", "import", "http", "url", "file", "get",
						 "photograph", "picture", "bitmap", "texture", "icon",
						 "png",
						 "jpeg", "jpg",
						 "gif",
						 "bmp",
						 "OpenEXR", "exr", "ilm",
						 "hdr", "high", "dynamic", "range",
						 "psd", "Photoshop",
						 "raw", "cr2", "dng", "dcr", "nef", "raf", "mos", "kdc",
						 "tiff",
						 "Targa", "tga"
					 ],
					 "version" : "2.0.0",
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
		VuoInputData(VuoText, {"default":"", "name":"URL"}) url,
		VuoOutputData(VuoImage) image
)
{
	*image = VuoImage_get(url);
}
