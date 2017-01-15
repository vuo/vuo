/**
 * @file
 * VuoSceneObject implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoSceneObject.h"
#include "VuoList_VuoImage.h"
#include "VuoList_VuoSceneObject.h"
#include "VuoBoolean.h"
#include "VuoFont.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Scene Object",
					 "description" : "A 3D Object: visible (mesh), or virtual (group, light, camera).",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoBlendMode",
						"VuoBoolean",
						"VuoFont",
						"VuoMesh",
						"VuoPoint3d",
						"VuoShader",
						"VuoText",
						"VuoTransform",
						"VuoList_VuoImage",
						"VuoList_VuoSceneObject"
					 ]
				 });
#endif
/// @}


/**
 * Creates a new, empty scene object.
 */
VuoSceneObject VuoSceneObject_makeEmpty(void)
{
	VuoSceneObject o;

	o.type = VuoSceneObjectType_Empty;

	o.mesh = NULL;
	o.shader = NULL;
	o.isRealSize = false;
	o.blendMode = VuoBlendMode_Normal;

	o.childObjects = NULL;

	o.name = NULL;
	o.transform = VuoTransform_makeIdentity();

	o.text = NULL;
	o.font = (VuoFont){NULL, 0, false, (VuoColor){0,0,0,0}, VuoHorizontalAlignment_Left, 0, 0};

	return o;
}

/**
 * Creates a new scene object that can contain (and transform) other scene objects, but doesn't render anything itself.
 */
VuoSceneObject VuoSceneObject_makeGroup(VuoList_VuoSceneObject childObjects, VuoTransform transform)
{
	VuoSceneObject o;

	o.type = VuoSceneObjectType_Group;

	o.mesh = NULL;
	o.shader = NULL;
	o.isRealSize = false;
	o.blendMode = VuoBlendMode_Normal;

	o.childObjects = childObjects;

	o.name = NULL;
	o.transform = transform;

	o.text = NULL;
	o.font = (VuoFont){NULL, 0, false, (VuoColor){0,0,0,0}, VuoHorizontalAlignment_Left, 0, 0};

	return o;
}

/**
 * Creates a visible (mesh) scene object.
 */
VuoSceneObject VuoSceneObject_make(VuoMesh mesh, VuoShader shader, VuoTransform transform, VuoList_VuoSceneObject childObjects)
{
	VuoSceneObject o;

	o.type = VuoSceneObjectType_Mesh;

	o.mesh = mesh;

	if (mesh && !shader)
		o.shader = VuoShader_makeDefaultShader();
	else
		o.shader = shader;

	o.isRealSize = false;
	o.blendMode = VuoBlendMode_Normal;

	o.childObjects = childObjects;

	o.name = NULL;

	o.transform = transform;

	o.text = NULL;
	o.font = (VuoFont){NULL, 0, false, (VuoColor){0,0,0,0}, VuoHorizontalAlignment_Left, 0, 0};

	return o;
}

/**
 * Returns a scene object that renders a quad with the specified shader.
 *
 * The quad does not include normals, tangents, or bitangents.
 *
 * @param shader The shader used to render the object.
 * @param center The object's center, specified in scene coordinates.
 * @param rotation The object's rotation, specified in degrees.
 * @param width The object's width, specified in scene coordinates.
 * @param height The object's height, specified in scene coordinates.
 * @return The quad scene object.
 *
 * @threadAnyGL
 */
VuoSceneObject VuoSceneObject_makeQuad(VuoShader shader, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal height)
{
	return VuoSceneObject_make(
				VuoMesh_makeQuadWithoutNormals(),
				shader,
				VuoTransform_makeEuler(
					center,
					VuoPoint3d_multiply(rotation, M_PI/180.),
					VuoPoint3d_make(width,height,1)
				),
				NULL
			);
}

/**
 * Returns a scene object that renders a quad with the specified shader.
 *
 * The quad includes normals, tangents, or bitangents.
 *
 * @param shader The shader used to render the object.
 * @param center The object's center, specified in scene coordinates.
 * @param rotation The object's rotation, specified in degrees.
 * @param width The object's width, specified in scene coordinates.
 * @param height The object's height, specified in scene coordinates.
 * @return The quad scene object.
 *
 * @threadAnyGL
 */
VuoSceneObject VuoSceneObject_makeQuadWithNormals(VuoShader shader, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal height)
{
	return VuoSceneObject_make(
				VuoMesh_makeQuad(),
				shader,
				VuoTransform_makeEuler(
					center,
					VuoPoint3d_multiply(rotation, M_PI/180.),
					VuoPoint3d_make(width,height,1)
				),
				NULL
			);
}

/**
 * Returns an unlit scene object with the specified @c image.
 *
 * @threadAnyGL
 */
VuoSceneObject VuoSceneObject_makeImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal alpha)
{
	if (!image)
		return VuoSceneObject_makeEmpty();

	VuoSceneObject object = VuoSceneObject_makeQuad(
				VuoShader_makeUnlitImageShader(image, alpha),
				center,
				rotation,
				width,
				image->pixelsHigh * width/image->pixelsWide
			);

	return object;
}

/**
 * Returns a lit scene object with the specified @c image.
 *
 * @threadAnyGL
 */
VuoSceneObject VuoSceneObject_makeLitImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal alpha, VuoColor highlightColor, VuoReal shininess)
{
	if (!image)
		return VuoSceneObject_makeEmpty();

	return VuoSceneObject_makeQuadWithNormals(
				VuoShader_makeLitImageShader(image, alpha, highlightColor, shininess),
				center,
				rotation,
				width,
				image->pixelsHigh * width/image->pixelsWide
			);
}

/**
 * Returns a scene object consisting of 6 child objects (square quads), each with its own shader.
 */
VuoSceneObject VuoSceneObject_makeCube(VuoTransform transform, VuoShader frontShader, VuoShader leftShader, VuoShader rightShader, VuoShader backShader, VuoShader topShader, VuoShader bottomShader)
{
	VuoList_VuoSceneObject cubeChildObjects = VuoListCreate_VuoSceneObject();

	VuoMesh quadMesh = VuoMesh_makeQuad();

	// Front Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadMesh,
					frontShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,0,.5), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Left Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadMesh,
					leftShader,
					VuoTransform_makeEuler(VuoPoint3d_make(-.5,0,0), VuoPoint3d_make(0,-M_PI/2.,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Right Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadMesh,
					rightShader,
					VuoTransform_makeEuler(VuoPoint3d_make(.5,0,0), VuoPoint3d_make(0,M_PI/2.,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Back Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadMesh,
					backShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,0,-.5), VuoPoint3d_make(0,M_PI,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Top Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadMesh,
					topShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,.5,0), VuoPoint3d_make(-M_PI/2.,0,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Bottom Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadMesh,
					bottomShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,-.5,0), VuoPoint3d_make(M_PI/2.,0,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	return VuoSceneObject_make(NULL, NULL, transform, cubeChildObjects);
}

/**
 * Returns a scene object representing deferred-rendered text.
 *
 * @threadAny
 */
VuoSceneObject VuoSceneObject_makeText(VuoText text, VuoFont font)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.type = VuoSceneObjectType_Text;
	o.text = text;
	o.font = font;
	return o;
}

/**
 * Returns a perspective camera having the position and negative-rotation specified by @c transform (its scale is ignored).
 */
VuoSceneObject VuoSceneObject_makePerspectiveCamera(VuoText name, VuoTransform transform, float fieldOfView, float distanceMin, float distanceMax)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.type = VuoSceneObjectType_PerspectiveCamera;
	o.name = name;
	o.transform = transform;
	o.cameraFieldOfView = fieldOfView;
	o.cameraDistanceMin = distanceMin;
	o.cameraDistanceMax = distanceMax;
	return o;
}

/**
 * Returns a stereoscopic camera having the position and negative-rotation specified by @c transform (its scale is ignored).
 */
VuoSceneObject VuoSceneObject_makeStereoCamera(VuoText name, VuoTransform transform, VuoReal fieldOfView, VuoReal distanceMin, VuoReal distanceMax, VuoReal confocalDistance, VuoReal intraocularDistance)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.type = VuoSceneObjectType_StereoCamera;
	o.name = name;
	o.transform = transform;
	o.cameraFieldOfView = fieldOfView;
	o.cameraDistanceMin = distanceMin;
	o.cameraDistanceMax = distanceMax;
	o.cameraConfocalDistance = confocalDistance;
	o.cameraIntraocularDistance = intraocularDistance;
	return o;
}

/**
 * Returns an orthographic camera having the position and negative-rotation specified by @c transform (its scale is ignored).
 */
VuoSceneObject VuoSceneObject_makeOrthographicCamera(VuoText name, VuoTransform transform, float width, float distanceMin, float distanceMax)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.type = VuoSceneObjectType_OrthographicCamera;
	o.name = name;
	o.transform = transform;
	o.cameraWidth = width;
	o.cameraDistanceMin = distanceMin;
	o.cameraDistanceMax = distanceMax;
	return o;
}

/**
 * Returns a fisheye camera having the position and negative-rotation specified by @c transform (its scale is ignored).
 */
VuoSceneObject VuoSceneObject_makeFisheyeCamera(VuoText name, VuoTransform transform, VuoReal fieldOfView, VuoReal vignetteWidth, VuoReal vignetteSharpness)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.type = VuoSceneObjectType_FisheyeCamera;
	o.name = name;
	o.transform = transform;
	o.cameraFieldOfView = fieldOfView;

	// 0 and 1000 come from "Realtime Dome Imaging and Interaction" by Bailey/Clothier/Gebbie 2006.
	o.cameraDistanceMin = 0;
	o.cameraDistanceMax = 1000;

	o.cameraVignetteWidth = vignetteWidth;
	o.cameraVignetteSharpness = vignetteSharpness;

	return o;
}

/**
 * Returns a perspective camera at (0,0,1), facing along -z, 90 degree FOV, and clip planes at 0.1 and 10.0.
 */
VuoSceneObject VuoSceneObject_makeDefaultCamera(void)
{
	VuoTransform transform = VuoTransform_makeEuler(
				VuoPoint3d_make(0,0,1),
				VuoPoint3d_make(0,0,0),
				VuoPoint3d_make(1,1,1)
			);
	return VuoSceneObject_makePerspectiveCamera(
				VuoText_make("default camera"),
				transform,
				90,
				0.1,
				10.0
				);
}

/**
 * Searches the scenegraph (depth-first) for a scene object with the given name.
 *
 * @param so The root object of the scenegraph to search.
 * @param nameToMatch The name to search for.
 * @param[out] ancestorObjects The ancestors of @a foundObject, starting with the root of the scenegraph.
 * @param[out] foundObject The first matching scene object found.
 * @return True if a matching scene object was found.
 */
bool VuoSceneObject_find(VuoSceneObject so, VuoText nameToMatch, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject *foundObject)
{
	if (VuoText_areEqual(so.name, nameToMatch))
	{
		*foundObject = so;
		return true;
	}

	VuoListAppendValue_VuoSceneObject(ancestorObjects, so);

	unsigned long childObjectCount = (so.childObjects ? VuoListGetCount_VuoSceneObject(so.childObjects) : 0);
	for (unsigned long i = 1; i <= childObjectCount; ++i)
	{
		VuoSceneObject childObject = VuoListGetValue_VuoSceneObject(so.childObjects, i);
		if (VuoSceneObject_find(childObject, nameToMatch, ancestorObjects, foundObject))
			return true;
	}

	VuoListRemoveLastValue_VuoSceneObject(ancestorObjects);

	return false;
}

/**
 * Helper for @ref VuoSceneObject_apply.
 */
static bool VuoSceneObject_findCameraInternal(VuoSceneObject so, VuoText nameToMatch, VuoSceneObject *foundCamera, float modelviewMatrix[16])
{
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(so.transform, localModelviewMatrix);
	float compositeModelviewMatrix[16];
	VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);

	if ((so.type == VuoSceneObjectType_PerspectiveCamera
	  || so.type == VuoSceneObjectType_StereoCamera
	  || so.type == VuoSceneObjectType_OrthographicCamera
	  || so.type == VuoSceneObjectType_FisheyeCamera)
	  && (!nameToMatch || (so.name && nameToMatch && strstr(so.name, nameToMatch))))
	{
		*foundCamera = so;
		so.transform = VuoTransform_makeFromMatrix4x4(compositeModelviewMatrix);
		return true;
	}

	if (so.childObjects)
	{
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject childObject = VuoListGetValue_VuoSceneObject(so.childObjects, i);
			VuoSceneObject childCamera;
			bool foundChildCamera = VuoSceneObject_findCameraInternal(childObject, nameToMatch, &childCamera, compositeModelviewMatrix);
			if (foundChildCamera)
			{
				*foundCamera = childCamera;
				return true;
			}
		}
	}

	return false;
}

/**
 * Performs a depth-first search of the scenegraph.
 *
 * Returns (via `foundCamera`) the first camera whose name contains `nameToMatch` (or, if `nameToMatch` is emptystring or NULL, just returns the first camera),
 * with its transform altered to incorporate the transforms of its ancestor objects.
 * The returned boolean indicates whether a camera was found.
 * If no camera was found, `foundCamera` is unaltered.
 */
bool VuoSceneObject_findCamera(VuoSceneObject so, VuoText nameToMatch, VuoSceneObject *foundCamera)
{
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), localModelviewMatrix);

	return VuoSceneObject_findCameraInternal(so, nameToMatch, foundCamera, localModelviewMatrix);
}

/**
 * Performs a depth-first search of the scenegraph.
 *
 * Returns true if the scene object or any of its children have a non-empty type.
 */
bool VuoSceneObject_isPopulated(VuoSceneObject so)
{
	if (so.type != VuoSceneObjectType_Empty)
		return true;

	if (so.childObjects)
	{
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject childObject = VuoListGetValue_VuoSceneObject(so.childObjects, i);
			if (VuoSceneObject_isPopulated(childObject))
				return true;
		}
	}

	return false;
}

/**
 * Returns the `VuoSceneObjectType` corresponding with the `typeString`.  If none matches, returns VuoSceneObjectType_Empty.
 */
static VuoSceneObjectType VuoSceneObject_typeFromCString(const char *typeString)
{
	if (strcmp(typeString,"empty")==0)
		return VuoSceneObjectType_Empty;
	else if (strcmp(typeString,"group")==0)
		return VuoSceneObjectType_Group;
	else if (strcmp(typeString,"mesh")==0)
		return VuoSceneObjectType_Mesh;
	else if (strcmp(typeString,"camera-perspective")==0)
		return VuoSceneObjectType_PerspectiveCamera;
	else if (strcmp(typeString,"camera-stereo")==0)
		return VuoSceneObjectType_StereoCamera;
	else if (strcmp(typeString,"camera-orthographic")==0)
		return VuoSceneObjectType_OrthographicCamera;
	else if (strcmp(typeString,"camera-fisheye")==0)
		return VuoSceneObjectType_FisheyeCamera;
	else if (strcmp(typeString,"light-ambient")==0)
		return VuoSceneObjectType_AmbientLight;
	else if (strcmp(typeString,"light-point")==0)
		return VuoSceneObjectType_PointLight;
	else if (strcmp(typeString,"light-spot")==0)
		return VuoSceneObjectType_Spotlight;
	else if (strcmp(typeString,"text")==0)
		return VuoSceneObjectType_Text;

	return VuoSceneObjectType_Empty;
}

/**
 * Returns a string constant representing `type`.
 */
static const char * VuoSceneObject_cStringForType(VuoSceneObjectType type)
{
	switch (type)
	{
		case VuoSceneObjectType_Empty:
			return "empty";
		case VuoSceneObjectType_Group:
			return "group";
		case VuoSceneObjectType_Mesh:
			return "mesh";
		case VuoSceneObjectType_PerspectiveCamera:
			return "camera-perspective";
		case VuoSceneObjectType_StereoCamera:
			return "camera-stereo";
		case VuoSceneObjectType_OrthographicCamera:
			return "camera-orthographic";
		case VuoSceneObjectType_FisheyeCamera:
			return "camera-fisheye";
		case VuoSceneObjectType_AmbientLight:
			return "light-ambient";
		case VuoSceneObjectType_PointLight:
			return "light-point";
		case VuoSceneObjectType_Spotlight:
			return "light-spot";
		case VuoSceneObjectType_Text:
			return "text";
		default:
			return "empty";
	}
}

/**
 * @ingroup VuoSceneObject
 * Returns an ambient light with the specified @c color and @c brightness (typically between 0 and 1).
 */
VuoSceneObject VuoSceneObject_makeAmbientLight(VuoColor color, float brightness)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.type = VuoSceneObjectType_AmbientLight;
	o.lightColor = color;
	o.lightBrightness = brightness;
	return o;
}

/**
 * @ingroup VuoSceneObject
 * Returns a point light (uniform emission in all directions).
 *
 * @param color The light's color.
 * @param brightness The light's brightness multiplier (typically between 0 and 1).
 * @param position The light's position.
 * @param range The distance (in local coordinates) the light reaches.
 * @param sharpness The sharpness of the light's distance falloff.  0 means the light starts fading at distance 0 and ends at 2*lightRange.  1 means the falloff is instant.
 */
VuoSceneObject VuoSceneObject_makePointLight(VuoColor color, float brightness, VuoPoint3d position, float range, float sharpness)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.type = VuoSceneObjectType_PointLight;
	o.lightColor = color;
	o.lightBrightness = brightness;
	o.lightRange = range;
	o.lightSharpness = MAX(MIN(sharpness,1),0);
	o.transform = VuoTransform_makeEuler(position, VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1));
	return o;
}

/**
 * @ingroup VuoSceneObject
 * Returns a spot light (emists only in the specified direction).
 *
 * @param color The light's color.
 * @param brightness The light's brightness multiplier (typically between 0 and 1).
 * @param transform The position and direction of the light.  (The transform's scale is ignored.)
 * @param cone Width (in radians) of the light's cone.
 * @param range The distance (in local coordinates) the light reaches.
 * @param sharpness The sharpness of the light's distance/cone falloff.  0 means the light starts fading at distance/angle 0 and ends at 2*range or 2*cone.  1 means the falloff is instant.
 */
VuoSceneObject VuoSceneObject_makeSpotlight(VuoColor color, float brightness, VuoTransform transform, float cone, float range, float sharpness)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.type = VuoSceneObjectType_Spotlight;
	o.lightColor = color;
	o.lightBrightness = brightness;
	o.lightCone = cone;
	o.lightRange = range;
	o.lightSharpness = MAX(MIN(sharpness,1),0);
	o.transform = transform;
	return o;
}

/**
 * Helper for @ref VuoSceneObject_findLights.
 */
static void VuoSceneObject_findLightsRecursive(VuoSceneObject so, float modelviewMatrix[16], VuoList_VuoColor ambientColors, float *ambientBrightness, VuoList_VuoSceneObject pointLights, VuoList_VuoSceneObject spotLights)
{
	switch (so.type)
	{
		case VuoSceneObjectType_AmbientLight:
			VuoListAppendValue_VuoColor(ambientColors, so.lightColor);
			*ambientBrightness += so.lightBrightness;
			return;
		case VuoSceneObjectType_PointLight:
		{
			float localModelviewMatrix[16];
			VuoTransform_getMatrix(so.transform, localModelviewMatrix);
			float compositeModelviewMatrix[16];
			VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);
			so.transform = VuoTransform_makeFromMatrix4x4(compositeModelviewMatrix);

			VuoListAppendValue_VuoSceneObject(pointLights, so);
			return;
		}
		case VuoSceneObjectType_Spotlight:
		{
			float localModelviewMatrix[16];
			VuoTransform_getMatrix(so.transform, localModelviewMatrix);
			float compositeModelviewMatrix[16];
			VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);
			so.transform = VuoTransform_makeFromMatrix4x4(compositeModelviewMatrix);

			VuoListAppendValue_VuoSceneObject(spotLights, so);
			return;
		}
		default:
			break;
	}

	if (so.childObjects)
	{
		float localModelviewMatrix[16];
		VuoTransform_getMatrix(so.transform, localModelviewMatrix);
		float compositeModelviewMatrix[16];
		VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);

		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject childObject = VuoListGetValue_VuoSceneObject(so.childObjects, i);
			VuoSceneObject_findLightsRecursive(childObject, compositeModelviewMatrix, ambientColors, ambientBrightness, pointLights, spotLights);
		}
	}
}

/**
 * Finds and returns all the lights in the scene.
 *
 * If there are multiple ambient lights, returns the weighted (by alpha) average color and summed brightness.
 *
 * If there are no lights in the scene, returns some default lights.
 */
void VuoSceneObject_findLights(VuoSceneObject so, VuoColor *ambientColor, float *ambientBrightness, VuoList_VuoSceneObject *pointLights, VuoList_VuoSceneObject *spotLights)
{
	VuoList_VuoColor ambientColors = VuoListCreate_VuoColor();
	VuoRetain(ambientColors);

	*ambientBrightness = 0;
	*pointLights = VuoListCreate_VuoSceneObject();
	*spotLights = VuoListCreate_VuoSceneObject();

	float localModelviewMatrix[16];
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), localModelviewMatrix);

	VuoSceneObject_findLightsRecursive(so, localModelviewMatrix, ambientColors, ambientBrightness, *pointLights, *spotLights);

	if (!VuoListGetCount_VuoColor(ambientColors)
			&& !VuoListGetCount_VuoSceneObject(*pointLights)
			&& !VuoListGetCount_VuoSceneObject(*spotLights))
	{
		*ambientColor = VuoColor_makeWithRGBA(1,1,1,1);
		*ambientBrightness = 0.05;

		// https://en.wikipedia.org/wiki/Three-point_lighting

		VuoSceneObject keyLight = VuoSceneObject_makePointLight(VuoColor_makeWithRGBA(1,1,1,1), .70, VuoPoint3d_make(-1,1,1), 5, .5);
		VuoListAppendValue_VuoSceneObject(*pointLights, keyLight);

		VuoSceneObject fillLight = VuoSceneObject_makePointLight(VuoColor_makeWithRGBA(1,1,1,1), .2, VuoPoint3d_make(.5,0,1), 5, 0);
		VuoListAppendValue_VuoSceneObject(*pointLights, fillLight);

		VuoSceneObject backLight = VuoSceneObject_makePointLight(VuoColor_makeWithRGBA(1,1,1,1), .15, VuoPoint3d_make(1,.75,-.5), 5, 0);
		VuoListAppendValue_VuoSceneObject(*pointLights, backLight);
	}
	else
		*ambientColor = VuoColor_average(ambientColors);

	VuoRelease(ambientColors);
}

/**
 * Applies @c function to @c object and its child objects,
 * without preserving changes to objects.
 */
void VuoSceneObject_visit(const VuoSceneObject object, void (^function)(const VuoSceneObject currentObject))
{
	function(object);

	if (object.childObjects)
	{
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(object.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject o = VuoListGetValue_VuoSceneObject(object.childObjects, i);
			VuoSceneObject_visit(o, function);
		}
	}
}

/**
 * Helper for @ref VuoSceneObject_apply.
 */
static void VuoSceneObject_applyInternal(VuoSceneObject *object, void (^function)(VuoSceneObject *currentObject, float modelviewMatrix[16]), float modelviewMatrix[16])
{
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(object->transform, localModelviewMatrix);
	float compositeModelviewMatrix[16];
	VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);

	function(object, compositeModelviewMatrix);

	if (object->childObjects)
	{
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(object->childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject o = VuoListGetValue_VuoSceneObject(object->childObjects, i);
			VuoSceneObject_applyInternal(&o, function, compositeModelviewMatrix);
			VuoListSetValue_VuoSceneObject(object->childObjects, o, i);
		}
	}
}

/**
 * Applies @c function to @c object and its child objects,
 * and outputs the modified @c object.
 *
 * The value `modelviewMatrix` (which `VuoSceneObject_apply` passes to `function`)
 * is the cumulative transformation matrix (from `object` down to the `currentObject`).
 */
void VuoSceneObject_apply(VuoSceneObject *object, void (^function)(VuoSceneObject *currentObject, float modelviewMatrix[16]))
{
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), localModelviewMatrix);

	VuoSceneObject_applyInternal(object, function, localModelviewMatrix);
}

/**
 * Sets the @c faceCullingMode on @c object and its child objects.
 *
 * @c faceCullingMode can be @c GL_NONE (show both front and back faces),
 * @c GL_BACK (show only front faces),
 * or @c GL_FRONT (show only back faces).
 */
void VuoSceneObject_setFaceCullingMode(VuoSceneObject *object, unsigned int faceCullingMode)
{
	VuoSceneObject_apply(object, ^(VuoSceneObject *currentObject, float modelviewMatrix[16]){
							 if (currentObject->mesh)
								 for (unsigned long i = 0; i < currentObject->mesh->submeshCount; ++i)
									 currentObject->mesh->submeshes[i].faceCullingMode = faceCullingMode;
						 });
}

/**
 * Sets the `blendMode` on `object` and its child objects.
 *
 * Only the following @ref VuoBlendMode%s are supported:
 *
 *    - @ref VuoBlendMode_Normal
 *    - @ref VuoBlendMode_Multiply
 *    - @ref VuoBlendMode_DarkerComponent
 *    - @ref VuoBlendMode_LighterComponent
 *    - @ref VuoBlendMode_LinearDodge
 *    - @ref VuoBlendMode_Subtract
 */
void VuoSceneObject_setBlendMode(VuoSceneObject *object, VuoBlendMode blendMode)
{
	VuoSceneObject_apply(object, ^(VuoSceneObject *currentObject, float modelviewMatrix[16]){
		currentObject->blendMode = blendMode;
	});
}

/**
 * Makes a deep copy of @c object.
 * Each mesh is copied (see @ref VuoMesh_copy),
 * and each child object is copied.
 *
 * You can change attributes on the copy without affecting the original.
 *
 * @todo The shaders are not copied, so changes to the copy's shaders will affect both the original and the copy.
 */
VuoSceneObject VuoSceneObject_copy(const VuoSceneObject object)
{
	VuoSceneObject copiedObject = object;

	copiedObject.mesh = VuoMesh_copy(object.mesh);

	if (object.childObjects)
	{
		copiedObject.childObjects = VuoListCreate_VuoSceneObject();
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(object.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
			VuoListAppendValue_VuoSceneObject(copiedObject.childObjects, VuoSceneObject_copy(VuoListGetValue_VuoSceneObject(object.childObjects, i)));
	}

	copiedObject.name = VuoText_make(object.name);

	return copiedObject;
}

/**
 * Returns the bounds of a sceneobject including its children.
 */
static bool VuoSceneObject_boundsRecursive(const VuoSceneObject so, VuoBox *bounds, float matrix[16])
{
	// matrix parameter is the trickle down transformations
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(so.transform, localModelviewMatrix);

	float compositeModelviewMatrix[16];
	VuoTransform_multiplyMatrices4x4(localModelviewMatrix, matrix, compositeModelviewMatrix);

	bool foundBounds = VuoSceneObject_meshBounds(so, bounds, compositeModelviewMatrix);

	if(so.childObjects != NULL)
	{
		for(int i = 0; i < VuoListGetCount_VuoSceneObject(so.childObjects); i++)
		{
			VuoBox childBounds;
			bool childBoundsFound = VuoSceneObject_boundsRecursive(VuoListGetValue_VuoSceneObject(so.childObjects, i+1), &childBounds, compositeModelviewMatrix);

			if(childBoundsFound)
			{
				*bounds = foundBounds ? VuoBox_encapsulate(*bounds, childBounds) : childBounds;
				foundBounds = true;
			}
		}
	}

	return foundBounds;
}

/**
  *	Get the axis aligned bounding box of this sceneobject and it's children (and it's children's children).
  */
VuoBox VuoSceneObject_bounds(const VuoSceneObject so)
{
	VuoBox bounds;

	float identity[16] =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	bool success = VuoSceneObject_boundsRecursive(so, &bounds, identity);

	if(success)
		return bounds;
	else
		return VuoBox_make( (VuoPoint3d){0,0,0}, (VuoPoint3d){0,0,0} );
}

/**
 *	Bounding box of the vertices for this SceneObject (taking into account transform).
 */
bool VuoSceneObject_meshBounds(const VuoSceneObject so, VuoBox *bounds, float matrix[16])
{
	if (VuoSceneObject_getVertexCount(so) < 1)
		return false;

	if (so.isRealSize)
		// We don't know what the actual rendered size of the realSize layer will be,
		// but we can at least include its center point.
		*bounds = VuoBox_make(VuoPoint3d_make(matrix[12], matrix[13], matrix[14]), VuoPoint3d_make(0,0,0));
	else
	{
		*bounds = VuoMesh_bounds(so.mesh, matrix);

		if (so.shader)
		{
			bounds->size.x *= so.shader->objectScale;
			bounds->size.y *= so.shader->objectScale;
			bounds->size.z *= so.shader->objectScale;
		}
	}

	return true;
}

/**
 *	Make the bounds center of all vertices == {0,0,0}
 */
void VuoSceneObject_center(VuoSceneObject *so)
{
	VuoBox bounds = VuoSceneObject_bounds(*so);
	so->transform.translation = VuoPoint3d_subtract(so->transform.translation, bounds.center);
}

/**
 * Change the root sceneobject's transform such that the entire scenegraph renders within a 1x1x1 axis-aligned cube.
 * If the scenegraph has zero size (e.g., if it is empty, or if it consists entirely of Real Size Layers),
 * the transform is left unchanged.
 */
void VuoSceneObject_normalize(VuoSceneObject *so)
{
	VuoBox bounds = VuoSceneObject_bounds(*so);

	float scale = fmax(fmax(bounds.size.x, bounds.size.y), bounds.size.z);
	if (fabs(scale) < 0.00001)
		return;

	so->transform.scale       = VuoPoint3d_multiply(so->transform.scale,       1./scale);
	so->transform.translation = VuoPoint3d_multiply(so->transform.translation, 1./scale);
}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "mesh" : ... ,
 *     "shader" : ... ,
 *     "isRealSize" : false,
 *     "childObjects" : ...,
 *     "transform" : ...
 *   }
 * }
 *
 * @eg{
 *   {
 *     "type" : "camera-perspective",
 *     "cameraFieldOfView" : 90.0,
 *     "cameraDistanceMin" : 0.1,
 *     "cameraDistanceMax" : 10.0,
 *     "name" : ...,
 *     "transform" : ...
 *   }
 * }
 */
VuoSceneObject VuoSceneObject_makeFromJson(json_object *js)
{
	json_object *o = NULL;

	VuoSceneObjectType type = VuoSceneObjectType_Empty;
	if (json_object_object_get_ex(js, "type", &o))
		type = VuoSceneObject_typeFromCString(json_object_get_string(o));

	VuoMesh mesh = NULL;
	if (json_object_object_get_ex(js, "mesh", &o))
		mesh = VuoMesh_makeFromJson(o);

	VuoShader shader = NULL;
	if (json_object_object_get_ex(js, "shader", &o))
		shader = VuoShader_makeFromJson(o);

	bool isRealSize = false;
	if (json_object_object_get_ex(js, "isRealSize", &o))
		isRealSize = VuoBoolean_makeFromJson(o);

	VuoBlendMode blendMode = VuoBlendMode_Normal;
	if (json_object_object_get_ex(js, "blendMode", &o))
		blendMode = VuoBlendMode_makeFromJson(o);

	VuoList_VuoSceneObject childObjects = NULL;
	if (json_object_object_get_ex(js, "childObjects", &o))
		childObjects = VuoList_VuoSceneObject_makeFromJson(o);

	float cameraFieldOfView;
	if (json_object_object_get_ex(js, "cameraFieldOfView", &o))
		cameraFieldOfView = json_object_get_double(o);

	float cameraWidth;
	if (json_object_object_get_ex(js, "cameraWidth", &o))
		cameraWidth = json_object_get_double(o);

	float cameraDistanceMin;
	if (json_object_object_get_ex(js, "cameraDistanceMin", &o))
		cameraDistanceMin = json_object_get_double(o);

	float cameraDistanceMax;
	if (json_object_object_get_ex(js, "cameraDistanceMax", &o))
		cameraDistanceMax = json_object_get_double(o);

	float cameraConfocalDistance;
	if (json_object_object_get_ex(js, "cameraConfocalDistance", &o))
		cameraConfocalDistance = json_object_get_double(o);

	float cameraIntraocularDistance;
	if (json_object_object_get_ex(js, "cameraIntraocularDistance", &o))
		cameraIntraocularDistance = json_object_get_double(o);

	float cameraVignetteWidth;
	if (json_object_object_get_ex(js, "cameraVignetteWidth", &o))
		cameraVignetteWidth = json_object_get_double(o);

	float cameraVignetteSharpness;
	if (json_object_object_get_ex(js, "cameraVignetteSharpness", &o))
		cameraVignetteSharpness = json_object_get_double(o);

	VuoColor lightColor;
	if (json_object_object_get_ex(js, "lightColor", &o))
		lightColor = VuoColor_makeFromJson(o);

	float lightBrightness;
	if (json_object_object_get_ex(js, "lightBrightness", &o))
		lightBrightness = json_object_get_double(o);

	float lightCone;
	if (json_object_object_get_ex(js, "lightCone", &o))
		lightCone = json_object_get_double(o);

	float lightRange;
	if (json_object_object_get_ex(js, "lightRange", &o))
		lightRange = json_object_get_double(o);

	float lightSharpness;
	if (json_object_object_get_ex(js, "lightSharpness", &o))
		lightSharpness = json_object_get_double(o);

	VuoText name = NULL;
	if (json_object_object_get_ex(js, "name", &o))
		name = VuoText_makeFromJson(o);

	json_object_object_get_ex(js, "transform", &o);
	VuoTransform transform = VuoTransform_makeFromJson(o);

	VuoText text = NULL;
	if (json_object_object_get_ex(js, "text", &o))
		text = VuoText_makeFromJson(o);

	VuoFont font;
	if (json_object_object_get_ex(js, "font", &o))
		font = VuoFont_makeFromJson(o);


	switch (type)
	{
		case VuoSceneObjectType_Empty:
			return VuoSceneObject_makeEmpty();
		case VuoSceneObjectType_Group:
			return VuoSceneObject_makeGroup(childObjects, transform);
		case VuoSceneObjectType_Mesh:
		{
			VuoSceneObject o = VuoSceneObject_make(mesh, shader, transform, childObjects);
			o.isRealSize = isRealSize;
			o.blendMode = blendMode;
			o.name = name;
			return o;
		}
		case VuoSceneObjectType_PerspectiveCamera:
			return VuoSceneObject_makePerspectiveCamera(
						name,
						transform,
						cameraFieldOfView,
						cameraDistanceMin,
						cameraDistanceMax
						);
		case VuoSceneObjectType_StereoCamera:
			return VuoSceneObject_makeStereoCamera(
						name,
						transform,
						cameraFieldOfView,
						cameraDistanceMin,
						cameraDistanceMax,
						cameraConfocalDistance,
						cameraIntraocularDistance
						);
		case VuoSceneObjectType_OrthographicCamera:
			return VuoSceneObject_makeOrthographicCamera(
						name,
						transform,
						cameraWidth,
						cameraDistanceMin,
						cameraDistanceMax
						);
		case VuoSceneObjectType_FisheyeCamera:
			return VuoSceneObject_makeFisheyeCamera(
						name,
						transform,
						cameraFieldOfView,
						cameraVignetteWidth,
						cameraVignetteSharpness
						);
		case VuoSceneObjectType_AmbientLight:
			return VuoSceneObject_makeAmbientLight(lightColor, lightBrightness);
		case VuoSceneObjectType_PointLight:
			return VuoSceneObject_makePointLight(lightColor, lightBrightness, transform.translation, lightRange, lightSharpness);
		case VuoSceneObjectType_Spotlight:
			return VuoSceneObject_makeSpotlight(lightColor, lightBrightness, transform, lightCone, lightRange, lightSharpness);
		case VuoSceneObjectType_Text:
		{
			VuoSceneObject o = VuoSceneObject_makeText(text, font);
			o.transform = transform;
			return o;
		}
	}
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoSceneObject_getJson(const VuoSceneObject value)
{
	json_object *js = json_object_new_object();

	json_object_object_add(js, "type", json_object_new_string(VuoSceneObject_cStringForType(value.type)));

	switch (value.type)
	{
		case VuoSceneObjectType_Empty:
			break;

		case VuoSceneObjectType_Group:
		case VuoSceneObjectType_Mesh:
			if (value.mesh)
			{
				json_object *meshObject = VuoMesh_getJson(value.mesh);
				json_object_object_add(js, "mesh", meshObject);
			}

			if (value.shader)
			{
				json_object *shaderObject = VuoShader_getJson(value.shader);
				json_object_object_add(js, "shader", shaderObject);
			}

			json_object *isRealSizeObject = VuoBoolean_getJson(value.isRealSize);
			json_object_object_add(js, "isRealSize", isRealSizeObject);

			if (value.blendMode != VuoBlendMode_Normal)
			{
				json_object *blendModeObject = VuoBlendMode_getJson(value.blendMode);
				json_object_object_add(js, "blendMode", blendModeObject);
			}

			if (value.childObjects)
			{
				json_object *childObjectsObject = VuoList_VuoSceneObject_getJson(value.childObjects);
				json_object_object_add(js, "childObjects", childObjectsObject);
			}

			break;

		case VuoSceneObjectType_PerspectiveCamera:
		case VuoSceneObjectType_StereoCamera:
		case VuoSceneObjectType_OrthographicCamera:
		case VuoSceneObjectType_FisheyeCamera:
			if (value.type != VuoSceneObjectType_FisheyeCamera)
			{
				json_object_object_add(js, "cameraDistanceMin", json_object_new_double(value.cameraDistanceMin));
				json_object_object_add(js, "cameraDistanceMax", json_object_new_double(value.cameraDistanceMax));
			}

			if (value.type == VuoSceneObjectType_PerspectiveCamera
			 || value.type == VuoSceneObjectType_StereoCamera
			 || value.type == VuoSceneObjectType_FisheyeCamera)
				json_object_object_add(js, "cameraFieldOfView", json_object_new_double(value.cameraFieldOfView));

			if (value.type == VuoSceneObjectType_StereoCamera)
			{
				json_object_object_add(js, "cameraConfocalDistance", json_object_new_double(value.cameraConfocalDistance));
				json_object_object_add(js, "cameraIntraocularDistance", json_object_new_double(value.cameraIntraocularDistance));
			}

			if (value.type == VuoSceneObjectType_OrthographicCamera)
				json_object_object_add(js, "cameraWidth", json_object_new_double(value.cameraWidth));

			if (value.type == VuoSceneObjectType_FisheyeCamera)
			{
				json_object_object_add(js, "cameraVignetteWidth", json_object_new_double(value.cameraVignetteWidth));
				json_object_object_add(js, "cameraVignetteSharpness", json_object_new_double(value.cameraVignetteSharpness));
			}

			break;

		case VuoSceneObjectType_AmbientLight:
		case VuoSceneObjectType_PointLight:
		case VuoSceneObjectType_Spotlight:
			json_object_object_add(js, "lightColor", VuoColor_getJson(value.lightColor));
			json_object_object_add(js, "lightBrightness", json_object_new_double(value.lightBrightness));

			if (value.type == VuoSceneObjectType_PointLight
			 || value.type == VuoSceneObjectType_Spotlight)
			{
				json_object_object_add(js, "lightRange", json_object_new_double(value.lightRange));
				json_object_object_add(js, "lightSharpness", json_object_new_double(value.lightSharpness));
			}
			if (value.type == VuoSceneObjectType_Spotlight)
				json_object_object_add(js, "lightCone", json_object_new_double(value.lightCone));

			break;

		case VuoSceneObjectType_Text:
			if (value.text)
			{
				json_object *textObject = VuoText_getJson(value.text);
				json_object_object_add(js, "text", textObject);
			}

			json_object *fontObject = VuoFont_getJson(value.font);
			json_object_object_add(js, "font", fontObject);

			break;
	}

	if (value.name)
	{
		json_object *nameObject = VuoText_getJson(value.name);
		json_object_object_add(js, "name", nameObject);
	}

	if (value.type != VuoSceneObjectType_AmbientLight)
	{
		json_object *transformObject = VuoTransform_getJson(value.transform);
		json_object_object_add(js, "transform", transformObject);
	}

	return js;
}

/**
 * Returns the total number of vertices in the scene object (but not its descendants).
 */
unsigned long VuoSceneObject_getVertexCount(const VuoSceneObject value)
{
	if (!value.mesh)
		return 0;

	unsigned long vertexCount = 0;
	for (unsigned int i = 0; i < value.mesh->submeshCount; ++i)
		vertexCount += value.mesh->submeshes[i].vertexCount;

	return vertexCount;
}

/**
 * Returns the total number of element in the scene object (but not its descendants).
 */
unsigned long VuoSceneObject_getElementCount(const VuoSceneObject value)
{
	if (!value.mesh)
		return 0;

	unsigned long elementCount = 0;
	for (unsigned int i = 0; i < value.mesh->submeshCount; ++i)
		elementCount += value.mesh->submeshes[i].elementCount;

	return elementCount;
}

/**
 * Traverses the specified scenegraph and returns statistics about it.
 */
static void VuoSceneObject_getStatistics(const VuoSceneObject value, unsigned long *descendantCount, unsigned long *totalVertexCount, unsigned long *totalElementCount)
{
	unsigned long childObjectCount = 0;
	if (value.childObjects)
		childObjectCount = VuoListGetCount_VuoSceneObject(value.childObjects);
	*descendantCount += childObjectCount;
	*totalVertexCount += VuoSceneObject_getVertexCount(value);
	*totalElementCount += VuoSceneObject_getElementCount(value);

	for (unsigned long i = 1; i <= childObjectCount; ++i)
		VuoSceneObject_getStatistics(VuoListGetValue_VuoSceneObject(value.childObjects, i), descendantCount, totalVertexCount, totalElementCount);
}

/**
 * Returns a list of all unique shader names in the sceneobject and its descendants.
 */
static VuoList_VuoText VuoSceneObject_findShaderNames(VuoSceneObject object)
{
	// Exploit json_object's set-containing-only-unique-items data structure.
	__block json_object *names = json_object_new_object();
	VuoSceneObject_visit(object, ^(const VuoSceneObject currentObject){
							 if (currentObject.shader)
								json_object_object_add(names, currentObject.shader->name, NULL);
						 });

	VuoList_VuoText nameList = VuoListCreate_VuoText();
	json_object_object_foreach(names, key, val)
		VuoListAppendValue_VuoText(nameList, VuoText_make(key));
	json_object_put(names);
	return nameList;
}

/**
 * Produces a brief human-readable summary of `value`.
 */
char *VuoSceneObject_getSummary(const VuoSceneObject value)
{
	if (value.type == VuoSceneObjectType_Text)
	{
		char *fontSummary = VuoFont_getSummary(value.font);
		char *textSummary = VuoText_format("\"%s\"<br>%sat (%g,%g)", value.text, fontSummary, value.transform.translation.x, value.transform.translation.y);
		free(fontSummary);
		return textSummary;
	}

	if (value.type == VuoSceneObjectType_PerspectiveCamera
	 || value.type == VuoSceneObjectType_StereoCamera
	 || value.type == VuoSceneObjectType_OrthographicCamera
	 || value.type == VuoSceneObjectType_FisheyeCamera)
	{
		const char *type = VuoSceneObject_cStringForType(value.type);

		float cameraViewValue = 0;
		const char *cameraViewString = "";
		if (value.type == VuoSceneObjectType_PerspectiveCamera)
		{
			cameraViewValue = value.cameraFieldOfView;
			cameraViewString = "° field of view";
		}
		else if (value.type == VuoSceneObjectType_StereoCamera)
		{
			cameraViewValue = value.cameraFieldOfView;
			cameraViewString = "° field of view (stereoscopic)";
		}
		else if (value.type == VuoSceneObjectType_OrthographicCamera)
		{
			cameraViewValue = value.cameraWidth;
			cameraViewString = " unit width";
		}
		else if (value.type == VuoSceneObjectType_FisheyeCamera)
		{
			cameraViewValue = value.cameraFieldOfView;
			cameraViewString = "° field of view (fisheye)";
		}

		char *translationString = VuoPoint3d_getSummary(value.transform.translation);

		const char *rotationLabel;
		char *rotationString;
		if (value.transform.type == VuoTransformTypeEuler)
		{
			rotationLabel = "rotated";
			rotationString = VuoPoint3d_getSummary(VuoPoint3d_multiply(value.transform.rotationSource.euler, -180.f/M_PI));
		}
		else
		{
			rotationLabel = "target";
			rotationString = VuoPoint3d_getSummary(value.transform.rotationSource.target);
		}

		char *valueAsString = VuoText_format("%s<br>at (%s)<br>%s (%s)<br>%g%s<br>shows objects between depth %g and %g",
											 type, translationString, rotationLabel, rotationString, cameraViewValue, cameraViewString, value.cameraDistanceMin, value.cameraDistanceMax);
		free(rotationString);
		free(translationString);
		return valueAsString;
	}

	if (value.type == VuoSceneObjectType_AmbientLight
	 || value.type == VuoSceneObjectType_PointLight
	 || value.type == VuoSceneObjectType_Spotlight)
	{
		const char *type = VuoSceneObject_cStringForType(value.type);
		char *colorString = VuoColor_getSummary(value.lightColor);

		char *positionRangeString;
		if (value.type == VuoSceneObjectType_PointLight
		 || value.type == VuoSceneObjectType_Spotlight)
		{
			char *positionString = VuoPoint3d_getSummary(value.transform.translation);

			positionRangeString = VuoText_format("<br>position (%s)<br>range %g units (%g sharpness)",
												 positionString, value.lightRange, value.lightSharpness);

			free(positionString);
		}
		else
			positionRangeString = strdup("");

		char *directionConeString;
		if (value.type == VuoSceneObjectType_Spotlight)
		{
			VuoPoint3d direction = VuoTransform_getDirection(value.transform);
			char *directionString = VuoPoint3d_getSummary(direction);

			directionConeString = VuoText_format("<br>direction (%s)<br>cone %g°",
												 directionString, value.lightCone * 180./M_PI);

			free(directionString);
		}
		else
			directionConeString = strdup("");

		char *valueAsString = VuoText_format("%s<br>color (%s)<br>brightness %g%s%s",
											 type, colorString, value.lightBrightness, positionRangeString, directionConeString);

		free(directionConeString);
		free(positionRangeString);
		free(colorString);

		return valueAsString;
	}

	unsigned long vertexCount = VuoSceneObject_getVertexCount(value);
	unsigned long elementCount = VuoSceneObject_getElementCount(value);

	char *transform = VuoTransform_getSummary(value.transform);

	unsigned long childObjectCount = 0;
	if (value.childObjects)
		childObjectCount = VuoListGetCount_VuoSceneObject(value.childObjects);
	const char *childObjectPlural = childObjectCount == 1 ? "" : "s";

	char *descendants;
	if (childObjectCount)
	{
		unsigned long descendantCount = 0;
		unsigned long totalVertexCount = 0;
		unsigned long totalElementCount = 0;
		VuoSceneObject_getStatistics(value, &descendantCount, &totalVertexCount, &totalElementCount);
		const char *descendantPlural = descendantCount == 1 ? "" : "s";

		descendants = VuoText_format("<br>%ld descendant%s<br><br>total, including descendants:<br>%ld vertices, %ld elements",
									 descendantCount, descendantPlural, totalVertexCount, totalElementCount);
	}
	else
		descendants = strdup("");

	VuoList_VuoText shaderNames = VuoSceneObject_findShaderNames(value);
	VuoRetain(shaderNames);
	char *shaderNamesSummary;
	if (VuoListGetCount_VuoText(shaderNames))
	{
		VuoInteger shaderNameCount = VuoListGetCount_VuoText(shaderNames);
		const char *header = "<br><br>shaders:<ul>";
		VuoInteger shaderNameLength = strlen(header);
		for (VuoInteger i = 1; i <= shaderNameCount; ++i)
			shaderNameLength += strlen("<li>") + strlen(VuoListGetValue_VuoText(shaderNames, i)) + strlen("</li>");
		shaderNameLength += strlen("</ul>");

		shaderNamesSummary = (char *)malloc(shaderNameLength + 1);
		char *t = shaderNamesSummary;
		t = strcpy(t, header) + strlen(header);
		for (VuoInteger i = 1; i <= shaderNameCount; ++i)
		{
			t = strcpy(t, "<li>") + strlen("<li>");
			t = strcpy(t, VuoListGetValue_VuoText(shaderNames, i)) + strlen(VuoListGetValue_VuoText(shaderNames, i));
			t = strcpy(t, "</li>") + strlen("</li>");
		}
		t = strcpy(t, "</ul>");
	}
	else
		shaderNamesSummary = strdup("");
	VuoRelease(shaderNames);

	char *valueAsString = VuoText_format("%ld vertices, %ld elements<br><br>%s<br><br>%ld child object%s%s%s",
										 vertexCount, elementCount, transform, childObjectCount, childObjectPlural, descendants, shaderNamesSummary);

	free(descendants);
	free(transform);
	free(shaderNamesSummary);

	return valueAsString;
}

/**
 * Outputs information about the sceneobject (and its descendants).
 */
static void VuoSceneObject_dump_internal(const VuoSceneObject so, unsigned int level)
{
	for (unsigned int i=0; i<level; ++i)
		fprintf(stderr, "\t");

	fprintf(stderr, "%s: ", VuoSceneObject_cStringForType(so.type));
	if (so.type == VuoSceneObjectType_Mesh)
		fprintf(stderr, "%lu vertices, %lu elements, shader '%s' (%p)", VuoSceneObject_getVertexCount(so), VuoSceneObject_getElementCount(so), so.shader ? so.shader->name : "", so.shader);
	fprintf(stderr, "\n");

	if (so.childObjects)
	{
		unsigned int childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
		for (unsigned int i=1; i<=childObjectCount; ++i)
			VuoSceneObject_dump_internal(VuoListGetValue_VuoSceneObject(so.childObjects, i), level+1);
	}
}

/**
 * Outputs information about the sceneobject (and its descendants).
 */
void VuoSceneObject_dump(const VuoSceneObject so)
{
	VuoSceneObject_dump_internal(so,0);
}
