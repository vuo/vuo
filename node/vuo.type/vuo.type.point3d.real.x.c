/**
 * @file
 * vuo.type.point3d.real node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Convert 3D Point to Real (X)",
					 "keywords" : [ ],
					 "version" : "1.0.1"
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":0, "y":0, "z":0}, "name":"(X,Y,Z)"}) xyz,
		VuoOutputData(VuoReal) x
)
{
	*x = xyz.x;
}
