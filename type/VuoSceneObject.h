/**
 * @file
 * VuoSceneObject C type definition.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCubemap.h"
#include "VuoHeap.h"
#include "VuoText.h"
#include "VuoMesh.h"
#include "VuoShader.h"
#include "VuoTransform.h"
#include "VuoPoint3d.h"
#include "VuoBlendMode.h"
#include "VuoFont.h"
#include "VuoOrientation.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @{ List type.
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
	VuoSceneObjectSubType_Empty,
	VuoSceneObjectSubType_Group,
	VuoSceneObjectSubType_Mesh,
	VuoSceneObjectSubType_PerspectiveCamera,
	VuoSceneObjectSubType_StereoCamera,
	VuoSceneObjectSubType_OrthographicCamera,
	VuoSceneObjectSubType_FisheyeCamera,
	VuoSceneObjectSubType_AmbientLight,
	VuoSceneObjectSubType_PointLight,
	VuoSceneObjectSubType_Spotlight,
	VuoSceneObjectSubType_Text
} VuoSceneObjectSubType;

/**
 * A 3D Object: visible (mesh), or virtual (group, light, camera).
 *
 * @version200Changed{VuoSceneObject is now an opaque, heap-allocated type.
 * Please use the get/set methods instead of directly accessing the structure.
 *
 * Also, only objects of type Group can now have child objects.}
 */
typedef const struct { void *l; } * VuoSceneObject;

uint64_t VuoSceneObject_getNextId(void);

// Constructors
VuoSceneObject VuoSceneObject_makeEmpty(void);
VuoSceneObject VuoSceneObject_makeGroup(VuoList_VuoSceneObject childObjects, VuoTransform transform);
VuoSceneObject VuoSceneObject_makeMesh(VuoMesh mesh, VuoShader shader, VuoTransform transform);
VuoSceneObject VuoSceneObject_makeQuad(VuoShader shader, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal height);
VuoSceneObject VuoSceneObject_makeQuadWithNormals(VuoShader shader, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal height);
VuoSceneObject VuoSceneObject_makeImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal size, VuoOrientation fixed, VuoReal alpha);
VuoSceneObject VuoSceneObject_makeLitImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal size, VuoOrientation fixed, VuoReal alpha, VuoColor highlightColor, VuoReal shininess);
VuoSceneObject VuoSceneObject_makeCube(VuoTransform transform, VuoShader frontShader, VuoShader leftShader, VuoShader rightShader, VuoShader backShader, VuoShader topShader, VuoShader bottomShader);
VuoSceneObject VuoSceneObject_makeCube1(VuoTransform transform, VuoShader shader);
VuoSceneObject VuoSceneObject_makeCube_VuoShader(VuoTransform transform, VuoShader shader);
VuoSceneObject VuoSceneObject_makeCube_VuoImage(VuoTransform transform, VuoImage image);
VuoSceneObject VuoSceneObject_makeCube_VuoColor(VuoTransform transform, VuoColor color);
VuoSceneObject VuoSceneObject_makeCube_VuoCubemap(VuoTransform transform, VuoCubemap cubemap);
VuoSceneObject VuoSceneObject_makeCubeMulti(VuoTransform transform, VuoInteger columns, VuoInteger rows, VuoInteger slices, VuoShader front, VuoShader left, VuoShader right, VuoShader back, VuoShader top, VuoShader bottom);
VuoSceneObject VuoSceneObject_makeText(VuoText text, VuoFont font, VuoBoolean scaleWithScene, float wrapWidth);
VuoSceneObject VuoSceneObject_makePerspectiveCamera(VuoText name, VuoTransform transform, float fieldOfView, float distanceMin, float distanceMax);
VuoSceneObject VuoSceneObject_makeStereoCamera(VuoText name, VuoTransform transform, VuoReal fieldOfView, VuoReal distanceMin, VuoReal distanceMax, VuoReal confocalDistance, VuoReal intraocularDistance);
VuoSceneObject VuoSceneObject_makeOrthographicCamera(VuoText name, VuoTransform transform, float width, float distanceMin, float distanceMax);
VuoSceneObject VuoSceneObject_makeFisheyeCamera(VuoText name, VuoTransform transform, VuoReal fieldOfView, VuoReal vignetteWidth, VuoReal vignetteSharpness);
VuoSceneObject VuoSceneObject_makeDefaultCamera(void);
VuoSceneObject VuoSceneObject_makeAmbientLight(VuoColor color, float brightness);
VuoSceneObject VuoSceneObject_makePointLight(VuoColor color, float brightness, VuoPoint3d position, float range, float sharpness);
VuoSceneObject VuoSceneObject_makeSpotlight(VuoColor color, float brightness, VuoTransform transform, float cone, float range, float sharpness);

// Finders
bool VuoSceneObject_find(VuoSceneObject so, VuoText nameToMatch, VuoList_VuoSceneObject parentObjects, VuoSceneObject *foundObject) VuoWarnUnusedResult;
bool VuoSceneObject_findById(VuoSceneObject so, uint64_t idToMatch, VuoList_VuoSceneObject parentObjects, VuoSceneObject *foundObject) VuoWarnUnusedResult;
bool VuoSceneObject_findWithType(VuoSceneObject so, VuoSceneObjectSubType typeToMatch, VuoList_VuoSceneObject parentObjects, VuoSceneObject *foundObject) VuoWarnUnusedResult;
bool VuoSceneObject_findCamera(VuoSceneObject so, VuoText nameToMatch, VuoSceneObject *foundCamera) VuoWarnUnusedResult;
void VuoSceneObject_findLights(VuoSceneObject so, VuoColor *ambientColor, float *ambientBrightness, VuoList_VuoSceneObject *pointLights, VuoList_VuoSceneObject *spotLights);

bool VuoSceneObject_isPopulated(VuoSceneObject so);

void VuoSceneObject_visit(const VuoSceneObject object, bool (^function)(const VuoSceneObject currentObject, float modelviewMatrix[16]));
void VuoSceneObject_apply(VuoSceneObject object, void (^function)(VuoSceneObject currentObject, float modelviewMatrix[16]));

// Mutators
void VuoSceneObject_normalize(VuoSceneObject so);
void VuoSceneObject_center(VuoSceneObject so);
void VuoSceneObject_transform(VuoSceneObject object, VuoTransform transform);
void VuoSceneObject_translate(VuoSceneObject object, VuoPoint3d translation);
void VuoSceneObject_scale(VuoSceneObject object, VuoPoint3d scale);

// Getters
VuoSceneObjectSubType VuoSceneObject_getType(const VuoSceneObject object);
uint64_t VuoSceneObject_getId(const VuoSceneObject object);
VuoText VuoSceneObject_getName(const VuoSceneObject object);
VuoList_VuoSceneObject VuoSceneObject_getChildObjects(const VuoSceneObject object);
VuoBlendMode VuoSceneObject_getBlendMode(const VuoSceneObject object);
VuoMesh VuoSceneObject_getMesh(const VuoSceneObject object);
VuoTransform VuoSceneObject_getTransform(const VuoSceneObject object);
VuoPoint3d VuoSceneObject_getTranslation(const VuoSceneObject object);
VuoShader VuoSceneObject_getShader(const VuoSceneObject object);
bool VuoSceneObject_isRealSize(const VuoSceneObject object);
bool VuoSceneObject_shouldPreservePhysicalSize(const VuoSceneObject object);
VuoText VuoSceneObject_getText(const VuoSceneObject object);
VuoFont VuoSceneObject_getTextFont(const VuoSceneObject object);
bool VuoSceneObject_shouldTextScaleWithScene(const VuoSceneObject object);
float VuoSceneObject_getTextWrapWidth(const VuoSceneObject object);
float VuoSceneObject_getCameraFieldOfView(const VuoSceneObject object);
float VuoSceneObject_getCameraWidth(const VuoSceneObject object);
float VuoSceneObject_getCameraDistanceMin(const VuoSceneObject object);
float VuoSceneObject_getCameraDistanceMax(const VuoSceneObject object);
float VuoSceneObject_getCameraVignetteWidth(const VuoSceneObject object);
float VuoSceneObject_getCameraVignetteSharpness(const VuoSceneObject object);
float VuoSceneObject_getCameraIntraocularDistance(const VuoSceneObject object);
float VuoSceneObject_getCameraConfocalDistance(const VuoSceneObject object);
VuoColor VuoSceneObject_getLightColor(const VuoSceneObject object);
float VuoSceneObject_getLightBrightness(const VuoSceneObject object);
float VuoSceneObject_getLightRange(const VuoSceneObject object);
float VuoSceneObject_getLightSharpness(const VuoSceneObject object);
float VuoSceneObject_getLightCone(const VuoSceneObject object);

// Setters
void VuoSceneObject_setType(VuoSceneObject object, VuoSceneObjectSubType type);
void VuoSceneObject_setId(VuoSceneObject object, uint64_t id);
void VuoSceneObject_setName(VuoSceneObject object, VuoText name);
void VuoSceneObject_setChildObjects(VuoSceneObject object, VuoList_VuoSceneObject childObjects);
void VuoSceneObject_setMesh(VuoSceneObject object, VuoMesh mesh);
void VuoSceneObject_setTransform(VuoSceneObject object, VuoTransform transform);
void VuoSceneObject_setTranslation(VuoSceneObject object, VuoPoint3d translation);
void VuoSceneObject_setScale(VuoSceneObject object, VuoPoint3d scale);
void VuoSceneObject_setShader(VuoSceneObject object, VuoShader shader);
void VuoSceneObject_setFaceCulling(VuoSceneObject object, VuoMesh_FaceCulling faceCullingMode);
void VuoSceneObject_setBlendMode(VuoSceneObject object, VuoBlendMode blendMode);
void VuoSceneObject_setRealSize(VuoSceneObject object, bool isRealSize);
void VuoSceneObject_setPreservePhysicalSize(VuoSceneObject object, bool shouldPreservePhysicalSize);
void VuoSceneObject_setText(VuoSceneObject object, VuoText text);
void VuoSceneObject_setTextFont(VuoSceneObject object, VuoFont font);
void VuoSceneObject_setCameraFieldOfView(VuoSceneObject object, float fieldOfView);
void VuoSceneObject_setCameraDistanceMin(VuoSceneObject object, float distanceMin);
void VuoSceneObject_setCameraDistanceMax(VuoSceneObject object, float distanceMax);

VuoSceneObject VuoSceneObject_copy(const VuoSceneObject object);

VuoSceneObject VuoSceneObject_makeFromJson(struct json_object * js);
struct json_object * VuoSceneObject_getJson(const VuoSceneObject value);
char * VuoSceneObject_getSummary(const VuoSceneObject value);
void VuoSceneObject_getStatistics(const VuoSceneObject value, unsigned long *descendantCount, unsigned long *totalVertexCount, unsigned long *totalElementCount);

VuoBox VuoSceneObject_bounds(const VuoSceneObject so);										///< Get the axis aligned bounding box of this sceneobject and it's children.
bool VuoSceneObject_meshBounds(const VuoSceneObject so, VuoBox *bounds, float matrix[16]); 	///< Bounding box of the vertices for this SceneObject (taking into account transform).
void VuoSceneObject_dump(const VuoSceneObject so);

unsigned long VuoSceneObject_getVertexCount(const VuoSceneObject value);

VuoSceneObject VuoSceneObject_flatten(const VuoSceneObject so);
VuoSceneObject VuoSceneObject_union(VuoList_VuoSceneObject objects, float quality);
VuoSceneObject VuoSceneObject_subtract(const VuoSceneObject a, const VuoSceneObject b, float quality);
VuoSceneObject VuoSceneObject_intersect(VuoList_VuoSceneObject objects, float quality);

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

#ifdef __cplusplus
}
#endif
