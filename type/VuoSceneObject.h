/**
 * @file
 * VuoSceneObject C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOSCENEOBJECT_H
#define VUOSCENEOBJECT_H

#include "VuoText.h"
#include "VuoVertices.h"
#include "VuoShader.h"
#include "VuoTransform.h"
#include "VuoList_VuoVertices.h"

/// @{
typedef void * VuoList_VuoSceneObject;
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
 * The type of camera
 */
typedef enum
{
	VuoSceneObject_NotACamera,
	VuoSceneObject_PerspectiveCamera,
	VuoSceneObject_OrthographicCamera
} VuoSceneObject_CameraType;

/**
 * The type of light
 */
typedef enum
{
	VuoSceneObject_NotALight,
	VuoSceneObject_AmbientLight,
	VuoSceneObject_PointLight,
	VuoSceneObject_Spotlight
} VuoSceneObject_LightType;

/**
 * A 3D Object: visible (mesh), or virtual (group, light, camera).
 */
typedef struct VuoSceneObject
{
	// Data for visible (mesh) scene objects
	VuoList_VuoVertices verticesList;
	VuoShader shader;
	bool isRealSize;	///< If the object is real-size, it ignores rotations and scales, and is sized to match the shader's first image.

	// Data for group scene objects
	VuoList_VuoSceneObject childObjects;

	// Data for camera scene objects
	VuoSceneObject_CameraType cameraType;
	float cameraFieldOfView;	///< Perspective FOV, in degrees.
	float cameraWidth;	///< Orthographic width, in scene coordinates.
	float cameraDistanceMin;	///< Distance from camera to near clip plane.
	float cameraDistanceMax;	///< Distance from camera to far clip plane.

	// Data for light scene objects
	VuoSceneObject_LightType lightType;
	VuoColor lightColor;
	float lightBrightness;
	float lightRange;	///< Distance (in local coordinates) the light reaches.  Affects point lights and spotlights.
	float lightCone;	///< Size (in radians) of the light's cone.  Affects spotlights.
	float lightSharpness;	///< Sharpness of the light's distance/cone falloff.  0 means the light starts fading at distance/angle 0 and ends at 2*lightRange or 2*lightCone.  1 means the falloff is instant.

	// Data for all scene objects
	VuoText name;
	VuoTransform transform;
} VuoSceneObject;

VuoSceneObject VuoSceneObject_makeEmpty(void);
VuoSceneObject VuoSceneObject_make(VuoList_VuoVertices verticesList, VuoShader shader, VuoTransform transform, VuoList_VuoSceneObject childObjects);
VuoSceneObject VuoSceneObject_makeQuad(VuoShader shader, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal height);
VuoSceneObject VuoSceneObject_makeQuadWithNormals(VuoShader shader, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal height);
VuoSceneObject VuoSceneObject_makeImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal alpha);
VuoSceneObject VuoSceneObject_makeLitImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal alpha, VuoColor highlightColor, VuoReal shininess);
VuoSceneObject VuoSceneObject_makeCube(VuoTransform transform, VuoShader frontShader, VuoShader leftShader, VuoShader rightShader, VuoShader backShader, VuoShader topShader, VuoShader bottomShader);

VuoSceneObject VuoSceneObject_makePerspectiveCamera(VuoText name, VuoTransform transform, float fieldOfView, float distanceMin, float distanceMax);
VuoSceneObject VuoSceneObject_makeOrthographicCamera(VuoText name, VuoTransform transform, float width, float distanceMin, float distanceMax);
VuoSceneObject VuoSceneObject_makeDefaultCamera(void);

bool VuoSceneObject_find(VuoSceneObject so, VuoText nameToMatch, VuoList_VuoSceneObject parentObjects, VuoSceneObject *foundObject);
VuoSceneObject VuoSceneObject_findCamera(VuoSceneObject so, VuoText nameToMatch, bool *foundCamera);

VuoSceneObject VuoSceneObject_makeAmbientLight(VuoColor color, float brightness);
VuoSceneObject VuoSceneObject_makePointLight(VuoColor color, float brightness, VuoPoint3d position, float range, float sharpness);
VuoSceneObject VuoSceneObject_makeSpotlight(VuoColor color, float brightness, VuoTransform transform, float cone, float range, float sharpness);

void VuoSceneObject_findLights(VuoSceneObject so, VuoColor *ambientColor, float *ambientBrightness, VuoList_VuoSceneObject *pointLights, VuoList_VuoSceneObject *spotLights);

VuoSceneObject VuoSceneObject_valueFromJson(struct json_object * js);
struct json_object * VuoSceneObject_jsonFromValue(const VuoSceneObject value);
char * VuoSceneObject_summaryFromValue(const VuoSceneObject value);

void VuoSceneObject_dump(const VuoSceneObject so);

///@{
/**
 * Automatically generated function.
 */
VuoSceneObject VuoSceneObject_valueFromString(const char *str);
char * VuoSceneObject_stringFromValue(const VuoSceneObject value);
void VuoSceneObject_retain(VuoSceneObject value);
void VuoSceneObject_release(VuoSceneObject value);
///@}

/**
 * @}
 */

#endif
