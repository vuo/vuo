/**
 * @file
 * vuo.type.integer.point3d node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Convert Integer to 3D Point (X,0,0)",
					 "keywords" : [ ],
					 "version" : "1.0.1"
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0}) x,
		VuoOutputData(VuoPoint3d, {"name":"(X,0,0)"}) point3d
)
{
	*point3d = VuoPoint3d_make(x, 0, 0);
}
