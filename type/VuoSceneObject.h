/**
 * @file
 * VuoSceneObject C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOSCENEOBJECT_H
#define VUOSCENEOBJECT_H

#include "VuoText.h"
#include "VuoMesh.h"
#include "VuoShader.h"
#include "VuoTransform.h"
#include "VuoPoint3d.h"
#include "VuoBlendMode.h"
#include "VuoFont.h"

/// @{
typedef const struct VuoList_VuoSceneObject_struct { void *l; } * VuoList_VuoSceneObject;
#define VuoList_VuoSceneObject_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoSceneObject VuoSceneObject
 * A 3D Object: visible (mesh), or virtual (group, light, camera).
 *
 * @{
 */

/**
 * How this scene object should be rendered or how it should affect other scene objects.
 */
typedef enum
{
	VuoSceneObjectType_Empty,
	VuoSceneObjectType_Group,
	VuoSceneObjectType_Mesh,
	VuoSceneObjectType_PerspectiveCamera,
	VuoSceneObjectType_StereoCamera,
	VuoSceneObjectType_OrthographicCamera,
	VuoSceneObjectType_FisheyeCamera,
	VuoSceneObjectType_AmbientLight,
	VuoSceneObjectType_PointLight,
	VuoSceneObjectType_Spotlight,
	VuoSceneObjectType_Text
} VuoSceneObjectType;

/**
 * A 3D Object: visible (mesh), or virtual (group, light, camera).
 */
typedef struct VuoSceneObject
{
	VuoSceneObjectType type;

	// Data for all scene objects
	VuoText name;
	VuoTransform transform;

	// Mesh
	VuoMesh mesh;
	VuoShader shader;
	bool isRealSize;	///< If the object is real-size, it ignores rotations and scales, and is sized to match the shader's first image.
	VuoBlendMode blendMode;

	// Group
	VuoList_VuoSceneObject childObjects;

	// Camera
	float cameraFieldOfView;	///< Perspective and fisheye FOV, in degrees.
	float cameraWidth;	///< Orthographic width, in scene coordinates.
	float cameraDistanceMin;	///< Distance from camera to near clip plane.
	float cameraDistanceMax;	///< Distance from camera to far clip plane.
	float cameraConfocalDistance;	///< Distance from camera to stereoscopic confocal plane.
	float cameraIntraocularDistance;	///< Distance between the stereoscopic camera pair.
	float cameraVignetteWidth;			///< Fisheye only.  Distance from the center of the viewport to the center of the vignette.
	float cameraVignetteSharpness;		///< Fisheye only.  Distance that the vignette gradient covers.

	// Light
	VuoColor lightColor;
	float lightBrightness;
	float lightRange;	///< Distance (in local coordinates) the light reaches.  Affects point lights and spotlights.
	float lightCone;	///< Size (in radians) of the light's cone.  Affects spotlights.
	float lightSharpness;	///< Sharpness of the light's distance/cone falloff.  0 means the light starts fading at distance/angle 0 and ends at 2*lightRange or 2*lightCone.  1 means the falloff is instant.

	// Text
	VuoText text;
	VuoFont font;
} VuoSceneObject;

VuoSceneObject VuoSceneObject_makeEmpty(void);
VuoSceneObject VuoSceneObject_makeGroup(VuoList_VuoSceneObject childObjects, VuoTransform transform);
VuoSceneObject VuoSceneObject_make(VuoMesh mesh, VuoShader shader, VuoTransform transform, VuoList_VuoSceneObject childObjects);
VuoSceneObject VuoSceneObject_makeQuad(VuoShader shader, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal height);
VuoSceneObject VuoSceneObject_makeQuadWithNormals(VuoShader shader, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal height);
VuoSceneObject VuoSceneObject_makeImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal alpha);
VuoSceneObject VuoSceneObject_makeLitImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal alpha, VuoColor highlightColor, VuoReal shininess);
VuoSceneObject VuoSceneObject_makeCube(VuoTransform transform, VuoShader frontShader, VuoShader leftShader, VuoShader rightShader, VuoShader backShader, VuoShader topShader, VuoShader bottomShader);
VuoSceneObject VuoSceneObject_makeText(VuoText text, VuoFont font);

VuoSceneObject VuoSceneObject_makePerspectiveCamera(VuoText name, VuoTransform transform, float fieldOfView, float distanceMin, float distanceMax);
VuoSceneObject VuoSceneObject_makeStereoCamera(VuoText name, VuoTransform transform, VuoReal fieldOfView, VuoReal distanceMin, VuoReal distanceMax, VuoReal confocalDistance, VuoReal intraocularDistance);
VuoSceneObject VuoSceneObject_makeOrthographicCamera(VuoText name, VuoTransform transform, float width, float distanceMin, float distanceMax);
VuoSceneObject VuoSceneObject_makeFisheyeCamera(VuoText name, VuoTransform transform, VuoReal fieldOfView, VuoReal vignetteWidth, VuoReal vignetteSharpness);
VuoSceneObject VuoSceneObject_makeDefaultCamera(void);

bool VuoSceneObject_find(VuoSceneObject so, VuoText nameToMatch, VuoList_VuoSceneObject parentObjects, VuoSceneObject *foundObject);
bool VuoSceneObject_findCamera(VuoSceneObject so, VuoText nameToMatch, VuoSceneObject *foundCamera);
bool VuoSceneObject_isPopulated(VuoSceneObject so);

VuoSceneObject VuoSceneObject_makeAmbientLight(VuoColor color, float brightness);
VuoSceneObject VuoSceneObject_makePointLight(VuoColor color, float brightness, VuoPoint3d position, float range, float sharpness);
VuoSceneObject VuoSceneObject_makeSpotlight(VuoColor color, float brightness, VuoTransform transform, float cone, float range, float sharpness);

void VuoSceneObject_findLights(VuoSceneObject so, VuoColor *ambientColor, float *ambientBrightness, VuoList_VuoSceneObject *pointLights, VuoList_VuoSceneObject *spotLights);

void VuoSceneObject_visit(VuoSceneObject object, void (^function)(VuoSceneObject currentObject));
void VuoSceneObject_apply(VuoSceneObject *object, void (^function)(VuoSceneObject *currentObject, float modelviewMatrix[16]));

void VuoSceneObject_setFaceCullingMode(VuoSceneObject *object, unsigned int faceCullingMode);
void VuoSceneObject_setBlendMode(VuoSceneObject *object, VuoBlendMode blendMode);

VuoSceneObject VuoSceneObject_copy(const VuoSceneObject object);

VuoSceneObject VuoSceneObject_makeFromJson(struct json_object * js);
struct json_object * VuoSceneObject_getJson(const VuoSceneObject value);
char * VuoSceneObject_getSummary(const VuoSceneObject value);

VuoBox VuoSceneObject_bounds(const VuoSceneObject so);										///< Get the axis aligned bounding box of this sceneobject and it's children.
bool VuoSceneObject_meshBounds(const VuoSceneObject so, VuoBox *bounds, float matrix[16]); 	///< Bounding box of the vertices for this SceneObject (taking into account transform).
void VuoSceneObject_normalize(VuoSceneObject *so);
void VuoSceneObject_center(VuoSceneObject *so);
void VuoSceneObject_dump(const VuoSceneObject so);

unsigned long VuoSceneObject_getVertexCount(const VuoSceneObject value);

///@{
/**
 * Automatically generated function.
 */
VuoSceneObject VuoSceneObject_makeFromString(const char *str);
char * VuoSceneObject_getString(const VuoSceneObject value);
void VuoSceneObject_retain(VuoSceneObject value);
void VuoSceneObject_release(VuoSceneObject value);
///@}

/**
 * @}
 */

#endif
