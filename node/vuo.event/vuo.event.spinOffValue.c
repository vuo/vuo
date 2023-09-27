/**
 * @file
 * vuo.data.spinOffValue node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Spin Off Value",
					 "keywords" : [ "scatter", "fork", "spawn", "thread", "multithread", "multicore", "parallel", "concurrent", "asynchronous", "background", "data" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "LoadImageAsynchronouslyAndShowUrl.vuo" ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoGenericType1) value,
	VuoOutputTrigger(spunOff, VuoGenericType1)
)
{
	spunOff(value);
}
