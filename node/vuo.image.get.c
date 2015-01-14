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
					 "description" :
						"<p>Loads or downloads an image from a URL.</p> \
						<p>To download an image from the internet, copy the image's URL from your browser. Example: <ul> \
						<li>http://vuo.org/sites/all/themes/vuo_theme/logo.png</li> \
						</ul></p> \
						<p>Currently, URLs that begin with 'https' are not supported.</p> \
						<p>To load an image from a file on the computer running the composition, create a URL from the image file's path. Examples: <ul> \
						<li>file:///System/Library/CoreServices/DefaultDesktop.jpg</li> \
						<li>file:///Users/me/file\\ with\\ spaces.png (for a file called `file with spaces.png`)</li> \
						</ul></p> \
						<p>To get a file's path in the correct format, open the Terminal application, \
						drag the file onto the Terminal window, and copy the path that appears in the Terminal window. \
						Add 'file://' at the beginning of the path.</p> \
						<p>Currently, only absolute file paths are supported. If you run a composition on a different computer \
						than it was created on, then the image files need to be in the same location as on the original computer.</p> \
						",
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
						 "isInterface" : true
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
