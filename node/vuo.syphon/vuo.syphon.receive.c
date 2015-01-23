/**
 * @file
 * vuo.syphon.receive node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"
#include "VuoSyphon.h"
#include <string.h>

VuoModuleMetadata({
					  "title" : "Receive Image via Syphon",
					  "keywords" : [ "application", "client", "frame", "get", "input", "interprocess", "IOSurface", "output", "share", "video" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : true,
						  "exampleCompositions" : [
							  "ReceiveImages.vuo",
							  "ReceiveImagesPreferablyFromVuo.vuo",
							  "ReceiveImagesOnlyFromVuo.vuo"
						  ]
					  },
					  "dependencies" : [
						"VuoSyphon"
					  ]
				  });


struct nodeInstanceData
{
	VuoSyphonClient *syphonClient;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->syphonClient = VuoSyphonClient_make();
	VuoRetain(context->syphonClient);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInputData(VuoSyphonServerDescription) serverDescription,
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedImage, VuoImage)
)
{
	VuoSyphonClient_connectToServer((*context)->syphonClient, serverDescription, receivedImage);
}

void nodeInstanceEvent
(
		VuoInputData(VuoSyphonServerDescription) serverDescription,
		VuoInputEvent(VuoPortEventBlocking_Wall, serverDescription) serverDescriptionEvent,
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedImage, VuoImage)
)
{
	if (serverDescriptionEvent)
		VuoSyphonClient_connectToServer((*context)->syphonClient, serverDescription, receivedImage);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoSyphonClient_disconnectFromServer((*context)->syphonClient);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->syphonClient);
}
