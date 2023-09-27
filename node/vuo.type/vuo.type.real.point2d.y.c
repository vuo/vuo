/**
 * @file
 * vuo.type.real.point2d node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Convert Real to 2D Point (0,Y)",
					 "keywords" : [ ],
					 "version" : "1.0.1"
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) y,
		VuoOutputData(VuoPoint2d, {"name":"(0,Y)"}) point2d
)
{
	*point2d = VuoPoint2d_make(0, y);
}
