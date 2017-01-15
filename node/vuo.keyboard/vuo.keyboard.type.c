/**
 * @file
 * vuo.keyboard.type node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoKeyboard.h"

VuoModuleMetadata({
					  "title" : "Receive Keyboard Typing",
					  "keywords" : [ "text", "string", "line", "word", "character" ],
					  "version" : "1.0.1",
					  "dependencies" : [ "VuoKeyboard" ],
					  "node": {
						  "isInterface" : true,
						  "exampleCompositions" : [ ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoKeyboard *typedListener;
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
		VuoInputData(VuoWindowReference) window,
		VuoOutputTrigger(typedLine, VuoText),
		VuoOutputTrigger(typedWord, VuoText),
		VuoOutputTrigger(typedCharacter, VuoText)
)
{
	(*context)->isTriggerStopped = false;
	VuoKeyboard_startListeningForTyping((*context)->typedListener, typedLine, typedWord, typedCharacter, window);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoOutputTrigger(typedLine, VuoText),
		VuoOutputTrigger(typedWord, VuoText),
		VuoOutputTrigger(typedCharacter, VuoText)
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoKeyboard_stopListening((*context)->typedListener);
	VuoKeyboard_startListeningForTyping((*context)->typedListener, typedLine, typedWord, typedCharacter, window);
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
