/**
 * @file
 * vuo.ui.make.interaction node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata(
{
	"title" : "Make Interaction",
	"keywords" : [ ],
	"version" : "1.0.0",
	"node" :
	{
		"exampleCompositions" : [ ]
	}
});

VuoInteraction* nodeInstanceInit()
{
	VuoInteraction* interaction = (VuoInteraction*) malloc(sizeof(VuoInteraction) * 1);
	VuoRegister(interaction, free);

	*interaction = VuoInteraction_make();

	return interaction;
}

void nodeInstanceEvent
(
	VuoInstanceData(VuoInteraction*) instance,
	VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}}) position,
	VuoInputEvent( {"eventBlocking":"wall", "data":"position" }) positionEvent,
	VuoInputData(VuoBoolean, {"default":false}) isPressed,
	VuoInputEvent( {"eventBlocking":"wall", "data":"isPressed" }) isPressedEvent,
	VuoOutputTrigger(interactionChanged, VuoWindowProperty)
)
{
	if( VuoInteraction_update(position, isPressed, *instance) )
	{
		VuoWindowProperty property;
		bzero(&property, sizeof(VuoWindowProperty));
		property.type = VuoWindowProperty_Interaction;
		property.interaction = *((VuoInteraction*)(*instance));
		interactionChanged(property);
	}
}
