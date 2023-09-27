/**
 * @file
 * vuo.point.within.circle node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Is within Circle",
					 "keywords" : [ "contains", "bounds", "limit", "test", "hit test", "distance", "radius", "diameter" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}}) point,
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}}) center,
		VuoInputData(VuoReal, {"default":1.0}) width,
		VuoOutputData(VuoBoolean) withinCircle
)
{
	*withinCircle = VuoPoint2d_distance(point, center) <= width / 2.;
}
