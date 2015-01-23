/**
 * @file
 * vuo.scene.frameRequest.get.frameCount node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Frame Request Count",
					 "keywords" : [],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoFrameRequest) frameRequest,
		VuoOutputData(VuoInteger) frameCount
)
{
	*frameCount = frameRequest.frameCount;
}
