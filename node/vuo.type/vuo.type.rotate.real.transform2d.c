/**
 * @file
 * vuo.type.rotate.real.transform2d node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Convert Real to Rotation",
					 "keywords" : [ "rotate", "matrix", "trs", "angle", "roll", "axis" ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":-180,"suggestedMax":180,"suggestedStep":15.0}) rotation,
		VuoOutputData(VuoTransform2d) transform
)
{
	// 2d transform stores rotation as a radian, and the constructor doesn't convert for ye'
	*transform = VuoTransform2d_make(VuoPoint2d_make(0,0), rotation * M_PI/180., VuoPoint2d_make(1,1));
}
