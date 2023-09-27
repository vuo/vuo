/**
 * @file
 * vuo.syphon.receive node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSyphon.h"
#include <string.h>

VuoModuleMetadata({
					  "title" : "Receive Syphon Video",
					  "keywords" : [ "application", "client", "frame", "get", "interprocess", "IOSurface", "output", "share" ],
					  "version" : "1.0.2",
					  "node": {
						  "exampleCompositions" : [ "ReceiveImages.vuo", "ReceiveImagesPreferablyFromVuo.vuo", "ReceiveImagesOnlyFromVuo.vuo" ]
					  },
					  "dependencies" : [
						"VuoSyphon"
					  ]
				  });


struct nodeInstanceData
{
	VuoSyphonClient *syphonClient;
	VuoSyphonServerDescription serverDescription;
	bool triggersEnabled;
};

static void updateServer(struct nodeInstanceData *context, VuoSyphonServerDescription newServerDescription, VuoOutputTrigger(receivedImage, VuoImage))
{
	VuoSyphonServerDescription_release(context->serverDescription);
	context->serverDescription = newServerDescription;
	VuoSyphonServerDescription_retain(context->serverDescription);

	VuoSyphonClient_connectToServer(context->syphonClient, newServerDescription, receivedImage);
}


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
	(*context)->triggersEnabled = true;
	updateServer(*context, serverDescription, receivedImage);
}

void nodeInstanceTriggerUpdate
(
	VuoInputData(VuoSyphonServerDescription) serverDescription,
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoOutputTrigger(receivedImage, VuoImage)
)
{
	updateServer(*context, serverDescription, receivedImage);
}

void nodeInstanceEvent
(
		VuoInputData(VuoSyphonServerDescription, {"name":"Server"}) serverDescription,
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedImage, VuoImage, {"eventThrottling":"drop"})
)
{
	if (!(*context)->triggersEnabled)
		return;

	if (! VuoSyphonServerDescription_areEqual(serverDescription, (*context)->serverDescription))
		updateServer(*context, serverDescription, receivedImage);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoSyphonClient_disconnectFromServer((*context)->syphonClient);
	(*context)->triggersEnabled = false;
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->syphonClient);
	VuoSyphonServerDescription_release((*context)->serverDescription);
}
