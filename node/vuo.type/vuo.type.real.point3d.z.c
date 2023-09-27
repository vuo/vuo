/**
 * @file
 * vuo.type.real.point3d node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Convert Real to 3D Point (0,0,Z)",
					 "keywords" : [ ],
					 "version" : "1.0.1"
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) z,
		VuoOutputData(VuoPoint3d, {"name":"(0,0,Z)"}) point3d
)
{
	*point3d = VuoPoint3d_make(0, 0, z);
}
