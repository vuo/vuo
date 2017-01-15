/**
 * @file
 * vuo.image.fetch.list node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageGet.h"

VuoModuleMetadata({
					 "title" : "Fetch List of Images",
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
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoImageGet"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoText, {"name":"URLs"}) urls,
		VuoOutputData(VuoList_VuoImage) images
)
{
	*images = VuoListCreate_VuoImage();

	unsigned long count = VuoListGetCount_VuoText(urls);
	for (unsigned long i = 1; i <= count; ++i)
	{
		VuoText url = VuoListGetValue_VuoText(urls, i);
		VuoImage image = VuoImage_get(url);
		if (image)
			VuoListAppendValue_VuoImage(*images, image);
	}
}
