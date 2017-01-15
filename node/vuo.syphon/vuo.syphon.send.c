/**
 * @file
 * vuo.syphon.send node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"
#include "VuoSyphon.h"
#include <string.h>
#include "string.h"

VuoModuleMetadata({
					  "title" : "Send Image via Syphon",
					  "keywords" : [ "application", "frame", "interprocess", "IOSurface", "server", "share", "video" ],
					  "version" : "1.0.1",
					  "node": {
						  "isInterface" : true,
						  "exampleCompositions" : [ "SendImages.vuo" ]
					  },
					  "dependencies" : [
						"VuoSyphon"
					  ]
				  });


struct nodeInstanceData
{
	VuoSyphonServer *syphonServer;
	VuoGlContext *glContext;
	VuoText serverName;
};

static void updateServer(struct nodeInstanceData *context, VuoText newServerName)
{
	VuoRelease(context->serverName);
	context->serverName = newServerName;
	VuoRetain(context->serverName);

	VuoSyphonServer_setName(context->syphonServer, newServerName);
}


struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoText) serverName
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->glContext = VuoGlContext_use();

	context->syphonServer = VuoSyphonServer_make(serverName, context->glContext);
	VuoRetain(context->syphonServer);

	context->serverName = serverName;
	VuoRetain(context->serverName);

	return context;
}

void nodeInstanceEvent
(
		VuoInputData(VuoText) serverName,
		VuoInputData(VuoImage) sendImage,
		VuoInputEvent({"eventBlocking":"none","data":"sendImage"}) sendImageEvent,
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if (! VuoText_areEqual(serverName, (*context)->serverName))
		updateServer(*context, serverName);

	if (sendImageEvent)
		VuoSyphonServer_publishFrame((*context)->syphonServer, sendImage);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->syphonServer);
	VuoGlContext_disuse((*context)->glContext);
	VuoRelease((*context)->serverName);
}
