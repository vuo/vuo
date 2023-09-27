/**
 * @file
 * vuo.video.make.videoframe node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoVideoFrame.h"

VuoModuleMetadata({
					 "title" : "Make Video Frame",
					  "keywords" : [
						  "timestamp"
					  ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions": [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, { "default": 0, "auto":"-inf", "autoSupersedesDefault":true } ) timestamp,
		VuoOutputData(VuoVideoFrame) videoFrame
)
{
	*videoFrame = VuoVideoFrame_make(image, timestamp, 0);
}
