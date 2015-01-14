/**
 * @file
 * vuo.scene.frameRequest.get.timestamp node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Frame Request Timestamp",
					 "description" :
						 "<p>Gives the time at which the requested frame will be displayed in a window.</p>",
					 "keywords" : [],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoFrameRequest) frameRequest,
		VuoOutputData(VuoReal) timestamp
)
{
	*timestamp = frameRequest.timestamp;
}
