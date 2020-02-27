/**
 * @file
 * vuo.keyboard.type node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoKeyboard.h"

VuoModuleMetadata({
					  "title" : "Receive Keyboard Typing",
					  "keywords" : [ "text", "string", "line", "word", "character" ],
					  "version" : "2.0.0",
					  "dependencies" : [ "VuoKeyboard" ],
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoKeyboard *typedListener;

	VuoWindowReference window;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->isTriggerStopped = true;
	context->typedListener = VuoKeyboard_make();
	VuoRetain(context->typedListener);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoOutputTrigger(typedLine, VuoText),
		VuoOutputTrigger(typedWord, VuoText),
		VuoOutputTrigger(typedCharacter, VuoText)
)
{
	(*context)->isTriggerStopped = false;

	VuoWindowReference w = NULL;
	if (VuoRenderedLayers_getWindow(window, &w))
		(*context)->window = w;

	VuoKeyboard_startListeningForTyping((*context)->typedListener, typedLine, typedWord, typedCharacter, (*context)->window);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoOutputTrigger(typedLine, VuoText),
		VuoOutputTrigger(typedWord, VuoText),
		VuoOutputTrigger(typedCharacter, VuoText)
)
{
	if ((*context)->isTriggerStopped)
		return;

	VuoWindowReference w = NULL;
	bool hasWindow = VuoRenderedLayers_getWindow(window, &w);
	if ((hasWindow && w != (*context)->window))
	{
		VuoKeyboard_stopListening((*context)->typedListener);

		(*context)->window = w;

		VuoKeyboard_startListeningForTyping((*context)->typedListener, typedLine, typedWord, typedCharacter, (*context)->window);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	VuoKeyboard_stopListening((*context)->typedListener);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->typedListener);
}
