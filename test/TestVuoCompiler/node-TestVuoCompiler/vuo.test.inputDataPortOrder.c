/**
 * @file
 * vuo.test.inputDataPortOrder node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "",
					 "description" : "",
					 "version" : "1.0.0",
				 });

void * nodeInstanceInit
(
		VuoInputData(VuoInteger, {"default":0}) inputData1,
		VuoInputData(VuoInteger, {"default":0}) inputData0,
		VuoInputData(VuoInteger, {"default":0}) inputData2
)
{
	return 0;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(void *) context,
		VuoInputData(VuoInteger, {"default":0}) inputData3,
		VuoInputData(VuoInteger, {"default":0}) inputData2,
		VuoInputData(VuoInteger, {"default":0}) inputData0
)
{
}

void nodeInstanceEvent
(
		VuoInstanceData(void *) context,
		VuoInputData(VuoInteger, {"default":0}) inputData0,
		VuoInputData(VuoInteger, {"default":0}) inputData1
)
{
}

void nodeInstanceFini
(
		VuoInstanceData(void *) context
)
{
}
