/**
 * @file
 * vuo.video.make.input node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoVideoInputDevice.h"

VuoModuleMetadata({
					 "title" : "Specify Video Input",
					  "keywords" : [
						  "animation",
						  "avi",
						  "dv", "dvc",
						  "h264", "h.264",
						  "mjpeg",
						  "mpeg", "m4v", "mp4",
						  "quicktime", "qt", "aic", "prores",
						  "video",
					  ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions": [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"", "name":"Name"}) name,
		VuoOutputData(VuoVideoInputDevice) device
)
{
	*device = VuoVideoInputDevice_make(VuoText_make(""), name);
}
