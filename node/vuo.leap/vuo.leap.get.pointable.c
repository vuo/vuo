/**
 * @file
 * vuo.leap.get.pointable node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapPointable.h"
#include "VuoLeapPointableType.h"

VuoModuleMetadata({
					  "title" : "Get Pointable Values",
					  "keywords" : [ "controller", "motion", "finger", "tool" ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  ],
					  "node": {
						  "isInterface" : false,
						  "exampleCompositions" : [ "DisplayLeapHand.vuo", "PlayFingerPuppetsWithLeap.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoLeapPointable) pointable,
		VuoOutputData(VuoInteger) id,
		VuoOutputData(VuoLeapPointableType) type,
		VuoOutputData(VuoReal) length,
		VuoOutputData(VuoReal) width,
		VuoOutputData(VuoPoint3d) direction,
		VuoOutputData(VuoPoint3d) tipPosition,
		VuoOutputData(VuoPoint3d) tipVelocity
)
{
	*id = pointable.id;
	*type = pointable.type;
	*length = pointable.length;
	*width = pointable.width;
	*direction	= pointable.direction;
	*tipPosition = pointable.tipPosition;
	*tipVelocity = pointable.tipVelocity;
}
