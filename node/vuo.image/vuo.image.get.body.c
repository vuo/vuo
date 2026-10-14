/**
 * @file
 * vuo.image.get.body node implementation.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoHumanBody.h"

VuoModuleMetadata({
	"title" : "Get Body Landmarks",
	"keywords" : [
		"humans", "person", "people",
		"left", "right", "side",
		"heads", "faces", "foreheads", "eyes", "eyebrows", "ears", "noses", "cheeks", "mouths", "lips", "chins", "necks",
		"shoulders", "arms", "elbows", "wrists", "hands", "fingers",
		"hips", "legs", "knees", "ankles", "foot", "feet", "heels", "toes",
		"skeletal", "skeleton", "joints", "bones", "pose",
	],
	"version" : "1.0.0",
	"node" : {
		"exampleCompositions" : [ "MoveDotWithShoulders.vuo", "StreamColorsFromWrists.vuo" ]
	}
});

void nodeEvent(
	VuoInputData(VuoHumanBody) body,
	VuoOutputData(VuoRectangle) rectangle,
	VuoOutputData(VuoPoint2d) leftEye,
	VuoOutputData(VuoPoint2d) rightEye,
	VuoOutputData(VuoPoint2d) nose,
	VuoOutputData(VuoPoint2d) leftEar,
	VuoOutputData(VuoPoint2d) rightEar,
	VuoOutputData(VuoPoint2d) neck,
	VuoOutputData(VuoPoint2d) leftShoulder,
	VuoOutputData(VuoPoint2d) rightShoulder,
	VuoOutputData(VuoPoint2d) leftElbow,
	VuoOutputData(VuoPoint2d) rightElbow,
	VuoOutputData(VuoPoint2d) leftWrist,
	VuoOutputData(VuoPoint2d) rightWrist,
	VuoOutputData(VuoPoint2d) leftHip,
	VuoOutputData(VuoPoint2d) rightHip,
	VuoOutputData(VuoPoint2d) leftKnee,
	VuoOutputData(VuoPoint2d) rightKnee,
	VuoOutputData(VuoPoint2d) leftAnkle,
	VuoOutputData(VuoPoint2d) rightAnkle
)
{
	struct json_object *hb = (struct json_object *)body;
	if (!hb || !json_object_is_type(hb, json_type_object))
	{
		*rectangle     = (VuoRectangle){{NAN, NAN}, {NAN, NAN}};
		*leftEye       = (VuoPoint2d){NAN, NAN};
		*rightEye      = (VuoPoint2d){NAN, NAN};
		*nose          = (VuoPoint2d){NAN, NAN};
		*leftEar       = (VuoPoint2d){NAN, NAN};
		*rightEar      = (VuoPoint2d){NAN, NAN};
		*neck          = (VuoPoint2d){NAN, NAN};
		*leftShoulder  = (VuoPoint2d){NAN, NAN};
		*rightShoulder = (VuoPoint2d){NAN, NAN};
		*leftElbow     = (VuoPoint2d){NAN, NAN};
		*rightElbow    = (VuoPoint2d){NAN, NAN};
		*leftWrist     = (VuoPoint2d){NAN, NAN};
		*rightWrist    = (VuoPoint2d){NAN, NAN};
		*leftHip       = (VuoPoint2d){NAN, NAN};
		*rightHip      = (VuoPoint2d){NAN, NAN};
		*leftKnee      = (VuoPoint2d){NAN, NAN};
		*rightKnee     = (VuoPoint2d){NAN, NAN};
		*leftAnkle     = (VuoPoint2d){NAN, NAN};
		*rightAnkle    = (VuoPoint2d){NAN, NAN};
		return;
	}

	*rectangle     = VuoRectangle_makeFromJson(json_object_object_get(hb, "rectangle"));
	*leftEye       = VuoPoint2d_makeFromJson(json_object_object_get(hb, "leftEye"));
	*rightEye      = VuoPoint2d_makeFromJson(json_object_object_get(hb, "rightEye"));
	*nose          = VuoPoint2d_makeFromJson(json_object_object_get(hb, "nose"));
	*leftEar       = VuoPoint2d_makeFromJson(json_object_object_get(hb, "leftEar"));
	*rightEar      = VuoPoint2d_makeFromJson(json_object_object_get(hb, "rightEar"));
	*neck          = VuoPoint2d_makeFromJson(json_object_object_get(hb, "neck"));
	*leftShoulder  = VuoPoint2d_makeFromJson(json_object_object_get(hb, "leftShoulder"));
	*rightShoulder = VuoPoint2d_makeFromJson(json_object_object_get(hb, "rightShoulder"));
	*leftElbow     = VuoPoint2d_makeFromJson(json_object_object_get(hb, "leftElbow"));
	*rightElbow    = VuoPoint2d_makeFromJson(json_object_object_get(hb, "rightElbow"));
	*leftWrist     = VuoPoint2d_makeFromJson(json_object_object_get(hb, "leftWrist"));
	*rightWrist    = VuoPoint2d_makeFromJson(json_object_object_get(hb, "rightWrist"));
	*leftHip       = VuoPoint2d_makeFromJson(json_object_object_get(hb, "leftHip"));
	*rightHip      = VuoPoint2d_makeFromJson(json_object_object_get(hb, "rightHip"));
	*leftKnee      = VuoPoint2d_makeFromJson(json_object_object_get(hb, "leftKnee"));
	*rightKnee     = VuoPoint2d_makeFromJson(json_object_object_get(hb, "rightKnee"));
	*leftAnkle     = VuoPoint2d_makeFromJson(json_object_object_get(hb, "leftAnkle"));
	*rightAnkle    = VuoPoint2d_makeFromJson(json_object_object_get(hb, "rightAnkle"));
}
