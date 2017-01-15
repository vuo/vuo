/**
 * @file
 * vuo.type.videoframe.real node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "../vuo.video/VuoVideoFrame.h"

VuoModuleMetadata({
					 "title" : "Convert Frame to Timestamp",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoVideoFrame) frame,
		VuoOutputData(VuoReal) timestamp
)
{
	*timestamp = frame.timestamp;
}
