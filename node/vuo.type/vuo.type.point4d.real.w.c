/**
 * @file
 * vuo.type.point4d.real node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Convert 4D Point to Real (W)",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoPoint4d, {"default":{"x":0, "y":0, "z":0, "w":0}, "name":"(X,Y,Z,W)"}) xyzw,
		VuoOutputData(VuoReal) w
)
{
	*w = xyzw.w;
}
