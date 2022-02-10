/**
 * @file
 * vuo.syphon.send node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoSyphon.h"
#include <string.h>
#include "string.h"

VuoModuleMetadata({
					  "title" : "Send Syphon Video",
					  "keywords" : [ "application", "frame", "interprocess", "IOSurface", "server", "share" ],
					  "version" : "1.0.1",
					  "node": {
						  "exampleCompositions" : [ "SendImages.vuo" ]
					  },
					  "dependencies" : [
						"VuoSyphon"
					  ]
				  });


struct nodeInstanceData
{
	VuoSyphonServer *syphonServer;
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

	context->syphonServer = VuoSyphonServer_make(serverName);
	VuoRetain(context->syphonServer);

	context->serverName = serverName;
	VuoRetain(context->serverName);

	return context;
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoImage) sendImage,
	VuoInputEvent({"eventBlocking":"none","data":"sendImage"}) sendImageEvent,
	VuoInputData(VuoText) serverName)
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
	VuoRelease((*context)->serverName);
}
