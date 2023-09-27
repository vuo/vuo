/**
 * @file
 * vuo.type.real.point4d node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Convert Real to 4D Point (0,0,0,W)",
					 "keywords" : [ ],
					 "version" : "1.0.1"
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) w,
		VuoOutputData(VuoPoint4d, {"name":"(0,0,0,W)"}) point4d
)
{
	*point4d = VuoPoint4d_make(0, 0, 0, w);
}
