/**
 * @file
 * vuo.video.get.videoframe node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoVideoFrame.h"

VuoModuleMetadata({
					 "title" : "Get Frame Values (Video)",
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
		VuoInputData(VuoVideoFrame) videoFrame,
		VuoOutputData(VuoImage) image,
		VuoOutputData(VuoReal) timestamp
)
{
	*image = videoFrame.image;
	*timestamp = videoFrame.timestamp;
}
