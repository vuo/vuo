/**
 * @file
 * vuo.video.info node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoVideo.h"
#include "VuoVideoOptimization.h"

VuoModuleMetadata({
					 "title" : "Get Movie Info",
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
					 "version" : "2.0.2",
					 "dependencies" : [
						 "VuoVideo",
						 "VuoUrl"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "SkimMovie.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"", "name":"URL"}) url,
		VuoOutputData(VuoReal) duration
//		VuoOutputData(VuoDictionary_VuoText) metadata
)
{
	VuoVideo decoder = VuoVideo_make(url, VuoVideoOptimization_Auto);

	if(decoder != NULL)
	{
		VuoRetain(decoder);
		*duration = VuoVideo_getDuration(decoder);
		VuoRelease(decoder);
	}
}
