/**
 * @file
 * vuo.event.increased2 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Increased",
					  "keywords" : [ "pulse", "watcher", "changed" ],
					  "version" : "2.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [
								  /* Sync with vuo.event.decreased2 */
								  "VuoInteger", /* VuoInteger first, so this node can be compiled when unspecialized, since it's a core type */
								  "VuoArtNetInputDevice",
								  "VuoArtNetOutputDevice",
								  "VuoBaudRate",
								  "VuoColor",
								  "VuoData",
								  "VuoDistribution3d",
								  "VuoDragEvent",
								  "VuoFont",
								  "VuoGridType",
								  "VuoHidControl",
								  "VuoHidDevice",
								  "VuoImageFormat",
								  "VuoMidiPitchBend",
								  "VuoMultisample",
								  "VuoNumberFormat",
								  "VuoOscInputDevice",
								  "VuoOscOutputDevice",
								  "VuoParity",
								  "VuoRange",
								  "VuoReal",
								  "VuoRectangle",
								  "VuoSerialDevice",
								  "VuoText",
								  "VuoTimeUnit",
								  "VuoTime",
								  "VuoUrl",
								  "VuoVertexAttribute",
								  "VuoWeekday",
							  ]
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				  });

struct nodeInstanceData
{
	bool first;
	VuoGenericType1 value;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->first = true;
	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoGenericType1) value,
		VuoInputEvent({"eventBlocking":"door","data":"value"}) valueEvent,
		VuoOutputEvent() increased
)
{
	if ((*context)->first)
	{
		*increased = true;

		(*context)->first = false;
		(*context)->value = value;
		VuoGenericType1_retain((*context)->value);
		return;
	}

	*increased = VuoGenericType1_isLessThan((*context)->value, value);

	VuoGenericType1_release((*context)->value);
	(*context)->value = value;
	VuoGenericType1_retain((*context)->value);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if (!(*context)->first)
		VuoGenericType1_release((*context)->value);
}
