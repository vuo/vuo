/**
 * @file
 * vuo.console.window node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>
#include <unistd.h>
#include <dispatch/dispatch.h>
#include <errno.h>

#include "VuoWindow.h"

VuoModuleMetadata({
					 "title" : "Display Console Window",
					 "keywords" : [ "text", "string", "print", "log", "write", "read", "input", "output", "type", "line", "word", "character", "debug", "troubleshoot" ],
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoWindow"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "HelloWorld.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoWindowText *window;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->window = VuoWindowText_make();
	VuoRetain(context->window);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(typedLine, VuoText),
		VuoOutputTrigger(typedWord, VuoText),
		VuoOutputTrigger(typedCharacter, VuoText)
)
{
	VuoWindowText_enableTriggers((*context)->window, typedLine, typedWord, typedCharacter);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoText, {"default":""}) writeLine,
		VuoInputEvent({"eventBlocking":"none","data":"writeLine"}) writeLineEvent
)
{
	if (writeLineEvent)
		VuoWindowText_appendLine((*context)->window, writeLine);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoWindowText_disableTriggers((*context)->window);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoWindowText_close((*context)->window);
	VuoRelease((*context)->window);
}
