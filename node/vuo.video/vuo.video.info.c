/**
 * @file
 * vuo.video.info node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMovie.h"

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
					 "version" : "2.0.1",
					 "dependencies" : [
						 "VuoMovie",
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
	VuoUrl normalizedUrl = VuoUrl_normalize(url, false);
	VuoRetain(normalizedUrl);

	VuoText path = VuoUrl_getPosixPath(normalizedUrl);
	VuoRetain(path);
	VuoRelease(normalizedUrl);

	double dur;
	if( VuoMovie_getInfo(path, &dur) )
		*duration = (VuoReal)dur;

	VuoRelease(path);
}
