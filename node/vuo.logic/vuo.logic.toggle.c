/**
 * @file
 * vuo.logic.toggle node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Toggle",
					 "keywords" : [ "event to boolean", "gate", "not", "!", "flip", "inverse", "reverse", "opposite", "alternate", "switch", "0", "1", "true", "false" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

VuoBoolean * nodeInstanceInit(void)
{
	VuoBoolean *state = (VuoBoolean *)malloc(sizeof(VuoBoolean));
	VuoRegister(state, free);
	*state = false;
	return state;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoBoolean *) state,
		VuoInputEvent({"eventBlocking":"none"}) toggle,
		VuoOutputData(VuoBoolean) value
)
{
	if (toggle)
	{
		**state = ! **state;
		*value = **state;
	}
}

void nodeInstanceFini
(
		VuoInstanceData(VuoBoolean *) state
)
{
}
