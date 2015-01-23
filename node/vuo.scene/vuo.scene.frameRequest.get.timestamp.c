/**
 * @file
 * vuo.scene.frameRequest.get.timestamp node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Frame Request Timestamp",
					 "keywords" : [],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false,
						 "exampleCompositions" : [ "DisplayScene.vuo", "SpinSphere.vuo" ]
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
