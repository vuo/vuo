/**
 * @file
 * vuo.console.window node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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
					 "description" :
						"<p>Displays a window that can show text and fire events when text is typed.</p> \
						<p>When the composition starts or this node is added to a running composition, it pops up a window that contains a text area.</p> \
						<p><ul> \
						<li>`writeLine` — When this port receives an event, its text is written to the window, followed by a linebreak. The text is appended at the end of any text already in the window.</li> \
						<li>`typedLine` — When a linebreak is typed in the window, this port fires an event with the line just completed.</li> \
						<li>`typedWord` — When a word boundary (whitespace following non-whitespace) is typed in the window, this port fires an event with the word just completed.</li> \
						<li>`typedCharacter` — When a character is typed in the window, this port fires an event with the character just typed.</li> \
						</ul></p>",
					 "keywords" : [ "text", "string", "print", "log", "write", "read", "input", "output", "type", "line", "word", "character" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoWindow"
					 ],
					 "node": {
						 "isInterface" : true
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
	VuoRegister(context->window, free);
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
		VuoInputEvent(VuoPortEventBlocking_None, writeLine) writeLineEvent
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
	VuoRelease((*context)->window);
}
