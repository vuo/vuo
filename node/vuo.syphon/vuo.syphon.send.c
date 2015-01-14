/**
 * @file
 * vuo.syphon.send node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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
					  "keywords" : [ "application", "frame", "interprocess", "IOSurface", "output", "server", "share", "video" ],
					  "version" : "1.0.0",
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
	VuoText *oldName;
};

struct nodeInstanceData * nodeInstanceInit
(
		VuoInputData(VuoText, "") serverName
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->glContext = VuoGlContext_use();

	context->syphonServer = VuoSyphonServer_make(serverName, context->glContext);
	VuoRetain(context->syphonServer);

	return context;
}

void nodeInstanceEvent
(
		VuoInputData(VuoText, "") serverName,
		VuoInputEvent(VuoPortEventBlocking_Wall, serverName) serverNameEvent,
		VuoInputData(VuoImage) sendImage,
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoText oldName = (*context)->oldName == NULL ? "" : *(*context)->oldName;

	if( serverName != NULL && strcmp(serverName, oldName) != 0 )
	{
		VuoSyphonServer_setName((*context)->syphonServer, serverName);

		VuoText name =  VuoText_make(serverName);

		VuoRelease((*context)->oldName);

		(*context)->oldName = malloc(sizeof(name));
		*(*context)->oldName = name;

		VuoRetain((*context)->oldName);
	}

	VuoSyphonServer_publishFrame((*context)->syphonServer, sendImage);
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->syphonServer);

	VuoGlContext_disuseSpecific((*context)->glContext);
}
