/**
 * @file
 * vuo.event.becameFalse2 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Became False",
					 "keywords" : [ "pulse", "watcher", "change", "boolean", "0", "1", "true" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

VuoBoolean * nodeInstanceInit(void)
{
	VuoBoolean *lastValue = (VuoBoolean *)malloc(sizeof(VuoBoolean));
	VuoRegister(lastValue, free);
	*lastValue = true;
	return lastValue;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoBoolean *) lastValue,
		VuoInputData(VuoBoolean, {"default":false}) value,
		VuoInputEvent({"eventBlocking":"door","data":"value"}) valueEvent,
		VuoOutputEvent() becameFalse
)
{
	*becameFalse = (**lastValue == true && value == false);
	**lastValue = value;
}

void nodeInstanceFini
(
		VuoInstanceData(VuoBoolean *) lastValue
)
{
}
