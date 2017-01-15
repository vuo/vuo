/**
 * @file
 * vuo.point.make.4d.quaternion node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Quaternion from Angle",
					 "keywords" : [ "homogenous", "xyzw", "rotation", "angle", "versor" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":1,"y":0,"z":0}}) axis,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":360,"suggestedStep":1}) angle,
		VuoOutputData(VuoPoint4d) quaternion
)
{
	*quaternion = VuoTransform_quaternionFromAxisAngle(axis, angle * M_PI/180.);
}
