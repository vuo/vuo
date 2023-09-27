/**
 * @file
 * vuo.ui.make.toggle node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRenderedLayers.h"
#include "VuoUiTheme.h"
#include "VuoUiThemeBase.hh"

VuoModuleMetadata({
					  "title" : "Make Toggle Button",
					  "keywords" : [
						  "gui", "user interface", "interact", "widget", "control",
						  "on", "turn", "off", "checkbox", "checkmark", "tick", "switch", "enable",
					  ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "ChangeColorWithButton.vuo", "DisplayControlPanel.vuo" ]
					  },
					  "dependencies": [
						  "VuoSceneText",
					  ],
				 });

struct nodeInstanceData
{
	bool first;
	VuoRenderedLayers renderedLayers;
	uint64_t id;

	struct {
		VuoText label;
		VuoAnchor anchor;
		VuoPoint2d position;
		VuoUiTheme theme;
		bool hovering;
		bool pressed;
		bool toggled;
	} prior;
};

extern "C" struct nodeInstanceData *nodeInstanceInit(
	VuoInputData(VuoBoolean) setValue)
{
	struct nodeInstanceData *instance = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->first = true;
	instance->renderedLayers = VuoRenderedLayers_makeEmpty();
	VuoRenderedLayers_retain(instance->renderedLayers);
	instance->id = VuoSceneObject_getNextId();
	instance->prior.toggled = setValue;

	return instance;
}

extern "C" void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoRenderedLayers) window,
	VuoInputData(VuoText, { "default" : "Toggle Button" }) label,
	VuoInputData(VuoBoolean, { "default" : false, "hasPortAction" : true }) setValue,
	VuoInputEvent({ "data" : "setValue", "hasPortAction" : true }) setValueEvent,
	VuoInputData(VuoAnchor, { "default" : { "horizontalAlignment" : "center", "verticalAlignment" : "center" } }) anchor,
	VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) position,
	VuoInputData(VuoUiTheme) theme,
	VuoOutputTrigger(updatedLayer, VuoLayer),
	VuoOutputTrigger(changed, VuoBoolean),
	VuoOutputTrigger(turnedOn, void),
	VuoOutputTrigger(turnedOff, void))
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*instance)->renderedLayers, window, &renderingDimensionsChanged);

	bool portChanged = false;
	if (!VuoText_areEqual((*instance)->prior.label, label))
	{
		portChanged = true;
		VuoRetain(label);
		VuoRelease((*instance)->prior.label);
		(*instance)->prior.label = label;
	}
	if (!VuoAnchor_areEqual((*instance)->prior.anchor, anchor))
	{
		portChanged = true;
		(*instance)->prior.anchor = anchor;
	}
	if (!VuoPoint2d_areEqual((*instance)->prior.position, position))
	{
		portChanged = true;
		(*instance)->prior.position = position;
	}
	if (!VuoUiTheme_areEqual((*instance)->prior.theme, theme))
	{
		portChanged = true;
		VuoRetain(theme);
		VuoRelease((*instance)->prior.theme);
		(*instance)->prior.theme = theme;
	}

	VuoUiThemeToggle *toggleTheme = static_cast<VuoUiThemeToggle *>(VuoUiTheme_getSpecificTheme(theme, "VuoUiThemeToggle"));
	VuoLocal(toggleTheme);

	bool isHovering = (*instance)->prior.hovering;
	bool isPressed  = (*instance)->prior.pressed;
	bool isToggled  = (*instance)->prior.toggled;

	if (setValueEvent)
	{
		isToggled = setValue;

		if (isToggled != (*instance)->prior.toggled)
		{
			changed(isToggled);
			if (isToggled)
				turnedOn();
			else
				turnedOff();
		}
	}

	if ((*instance)->first)
	{
		changed(isToggled);
		if (isToggled)
			turnedOn();
		else
			turnedOff();
	}

	VuoList_VuoInteraction interactions = VuoRenderedLayers_getInteractions((*instance)->renderedLayers);
	unsigned long interactionCount = VuoListGetCount_VuoInteraction(interactions);
	for (int i = 0; i < interactionCount; ++i)
	{
		VuoInteraction interaction = VuoListGetValue_VuoInteraction(interactions, i + 1);
		isHovering = VuoRenderedLayers_isPointInLayerId((*instance)->renderedLayers, (*instance)->id, interaction.position);

		if (interaction.type == VuoInteractionType_Press
			|| interaction.type == VuoInteractionType_DragStart)
		{
			if (isHovering)
				isPressed = true;
		}
		else if (interaction.type == VuoInteractionType_Release
				 || interaction.type == VuoInteractionType_Click
				 || interaction.type == VuoInteractionType_DragFinish)
		{
			if (isPressed && isHovering)
			{
				isToggled = !isToggled;
				changed(isToggled);
				if (isToggled)
					turnedOn();
				else
					turnedOff();
			}

			isPressed = false;
		}
		else if (interaction.type == VuoInteractionType_Canceled)
			isPressed = false;
	}


	if ((*instance)->first
		|| portChanged
		|| (isHovering != (*instance)->prior.hovering)
		|| (isPressed  != (*instance)->prior.pressed)
		|| (isToggled  != (*instance)->prior.toggled))
	{
		VuoLayer layer = toggleTheme->render((*instance)->renderedLayers, label, position, anchor, isHovering, isPressed, isToggled);
		if (layer)
		{
			(*instance)->first          = false;
			(*instance)->prior.hovering = isHovering;
			(*instance)->prior.pressed  = isPressed;
			(*instance)->prior.toggled  = isToggled;

			VuoLayer_setId(layer, (*instance)->id);

			updatedLayer(layer);
		}
	}
}

extern "C" void nodeInstanceFini(
	VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRenderedLayers_release((*instance)->renderedLayers);
	VuoRelease((*instance)->prior.label);
	VuoRelease((*instance)->prior.theme);
}
