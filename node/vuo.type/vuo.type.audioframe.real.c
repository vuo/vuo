/**
 * @file
 * vuo.type.audioframe.real node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoAudioFrame.h"

VuoModuleMetadata({
					 "title" : "Convert Frame to Timestamp",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoAudioFrame) frame,
		VuoOutputData(VuoReal) timestamp
)
{
	*timestamp = frame.timestamp;
}
