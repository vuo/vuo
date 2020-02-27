/**
 * @file
 * vuo.video.make.format node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMovieFormat.h"

VuoModuleMetadata({
					 "title" : "Make Movie Format",
					  "keywords" : [
						  "animation",
						  "avi",
						  "dv", "dvc",
						  "h264", "h.264",
						  "mjpeg",
						  "mpeg", "m4v", "mp4",
						  "quicktime", "qt", "aic", "prores",
						  "video", "encoding"
					  ],
					 "version" : "1.0.0",
				 });

void nodeEvent
(
		VuoInputData(VuoMovieImageEncoding, {"default":"JPEG", "name":"Image Encoding"}) imageEncoding,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":1, "name":"Image Quality"}) imageQuality,
		VuoInputData(VuoAudioEncoding, {"default":"LinearPCM", "name":"Audio Encoding"}) audioEncoding,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":0, "suggestedMax":1, "name":"Audio Quality"}) audioQuality,
		VuoOutputData(VuoMovieFormat) movieFormat
)
{
	*movieFormat = VuoMovieFormat_make(imageEncoding, imageQuality, audioEncoding, audioQuality);
}
