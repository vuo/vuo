/**
 * @file
 * vuo.osc.skeleton.basic node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoOscMessage.h"

#include <string.h>

VuoModuleMetadata({
					 "title" : "Filter Skeleton",
					 "keywords" : [
						 "skeletal", "tracking",
						 "Delicode NI mate 2", "nimate",
					 ],
					 "version" : "1.0.0",
					 "node": {
						 "isDeprecated": true,  // https://ni-mate.com/download/ says "This product is no longer for sale"
					 }
				 });

#define JOINTS 15

struct nodeInstanceData
{
	VuoInteger priorUser;
	char *strings[JOINTS];
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->priorUser = -1;

	return context;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) context,

	VuoInputData(VuoOscMessage) message,
	VuoInputEvent({"eventBlocking":"door","data":"message"}) messageEvent,
	VuoInputData(VuoInteger, {"default":0, "suggestedMin":0}) user,
	VuoInputEvent({"eventBlocking":"wall","data":"user"}) userEvent,

	VuoOutputData(VuoPoint3d) head,
	VuoOutputEvent({"data":"head"}) headEvent,
	VuoOutputData(VuoPoint3d) neck,
	VuoOutputEvent({"data":"neck"}) neckEvent,
	VuoOutputData(VuoPoint3d) torso,
	VuoOutputEvent({"data":"torso"}) torsoEvent,
	VuoOutputData(VuoPoint3d) rightShoulder,
	VuoOutputEvent({"data":"rightShoulder"}) rightShoulderEvent,
	VuoOutputData(VuoPoint3d) leftShoulder,
	VuoOutputEvent({"data":"leftShoulder"}) leftShoulderEvent,
	VuoOutputData(VuoPoint3d) rightElbow,
	VuoOutputEvent({"data":"rightElbow"}) rightElbowEvent,
	VuoOutputData(VuoPoint3d) leftElbow,
	VuoOutputEvent({"data":"leftElbow"}) leftElbowEvent,
	VuoOutputData(VuoPoint3d) rightHand,
	VuoOutputEvent({"data":"rightHand"}) rightHandEvent,
	VuoOutputData(VuoPoint3d) leftHand,
	VuoOutputEvent({"data":"leftHand"}) leftHandEvent,
	VuoOutputData(VuoPoint3d) rightHip,
	VuoOutputEvent({"data":"rightHip"}) rightHipEvent,
	VuoOutputData(VuoPoint3d) leftHip,
	VuoOutputEvent({"data":"leftHip"}) leftHipEvent,
	VuoOutputData(VuoPoint3d) rightKnee,
	VuoOutputEvent({"data":"rightKnee"}) rightKneeEvent,
	VuoOutputData(VuoPoint3d) leftKnee,
	VuoOutputEvent({"data":"leftKnee"}) leftKneeEvent,
	VuoOutputData(VuoPoint3d) rightFoot,
	VuoOutputEvent({"data":"rightFoot"}) rightFootEvent,
	VuoOutputData(VuoPoint3d) leftFoot,
	VuoOutputEvent({"data":"leftFoot"}) leftFootEvent
)
{
	if (!message || !message->address)
		return;

	const char *strings[JOINTS] = {"Head",    "Neck",    "Torso",    "Right_Shoulder",   "Left_Shoulder",   "Right_Elbow",    "Left_Elbow",  "Right_Hand",   "Left_Hand",   "Right_Hip",   "Left_Hip",   "Right_Knee",   "Left_Knee",   "Right_Foot",   "Left_Foot"   };
	VuoPoint3d *outputs[JOINTS] = { head,      neck,      torso,      rightShoulder,      leftShoulder,      rightElbow,      leftElbow,      rightHand,      leftHand,      rightHip,      leftHip,      rightKnee,      leftKnee,      rightFoot,      leftFoot     };
	bool  *outputEvents[JOINTS] = { headEvent, neckEvent, torsoEvent, rightShoulderEvent, leftShoulderEvent, rightElbowEvent, leftElbowEvent, rightHandEvent, leftHandEvent, rightHipEvent, leftHipEvent, rightKneeEvent, leftKneeEvent, rightFootEvent, leftFootEvent};

	if (strcmp(message->address, "/NI_mate_sync") == 0)
	{
		for (int i = 0; i < JOINTS; ++i)
			*(outputEvents[i]) = true;
		return;
	}

	if (user != (*context)->priorUser)
	{
		for (int i = 0; i < JOINTS; ++i)
		{
			free((*context)->strings[i]);
			if (user == 0)
				(*context)->strings[i] = strdup(strings[i]);
			else
				(*context)->strings[i] = VuoText_format("%s_%lld", strings[i], user);
		}

		(*context)->priorUser = user;
	}

	for (int i = 0; i < JOINTS; ++i)
		if (strcmp(message->address, (*context)->strings[i]) == 0)
		{
			*(outputs[i]) = (VuoPoint3d){ -VuoReal_makeFromJson(message->data[0]),
										   VuoReal_makeFromJson(message->data[1]),
										  -VuoReal_makeFromJson(message->data[2]) };
			return;
		}
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
	for (int i = 0; i < JOINTS; ++i)
		free((*context)->strings[i]);
}
