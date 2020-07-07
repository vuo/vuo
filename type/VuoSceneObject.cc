/**
 * @file
 * VuoSceneObject implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <list>

#include <OpenGL/CGLMacro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoMeshUtility.h"

/// @{
#ifdef VUO_COMPILER
extern "C"
{
VuoModuleMetadata({
					 "title" : "Scene Object",
					 "description" : "A 3D Object: visible (mesh), or virtual (group, light, camera).",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"csgjs",
						"VuoBlendMode",
						"VuoBoolean",
						"VuoCubemap",
						"VuoFont",
						"VuoMesh",
						"VuoMeshUtility",
						"VuoPoint3d",
						"VuoShader",
						"VuoText",
						"VuoTransform",
						"VuoList_VuoImage",
						"VuoList_VuoSceneObject"
					 ]
				 });
}
#endif
/// @}

/**
 * @private VuoSceneObject fields.
 *
 * When a constructor (e.g., `VuoSceneObject_make`) returns,
 * VuoSceneObject's component objects (e.g., `mesh`) should have retain count +1,
 * and the VuoSceneObject itself should have retain count 0.
 */
typedef struct
{
	VuoSceneObjectSubType type;

	// Data for all scene objects
	uint64_t id;  ///< A unique ID for this object.  Persists through livecoding reloads (but not through save/load).  Used by @ref VuoRenderedLayers.
	VuoText name;
	VuoTransform transform;

	// Mesh
	VuoMesh mesh;
	VuoShader shader;
	bool isRealSize;  ///< If the object is real-size, it ignores rotations and scales, and is sized to match the shader's first image.
	bool preservePhysicalSize;  ///< Only used if isRealSize=true.  If preservePhysicalSize=true, uses the texture's scaleFactor and the backingScaleFactor to determine the rendered size.  If preservePhysicalSize=false, the texture is always rendered 1:1.
	VuoBlendMode blendMode;

	union
	{
		VuoList_VuoSceneObject childObjects;

		struct
		{
			float fieldOfView;          ///< Perspective and fisheye FOV, in degrees.
			float width;                ///< Orthographic width, in scene coordinates.
			float distanceMin;          ///< Distance from camera to near clip plane.
			float distanceMax;          ///< Distance from camera to far clip plane.
			float confocalDistance;     ///< Distance from camera to stereoscopic confocal plane.
			float intraocularDistance;  ///< Distance between the stereoscopic camera pair.
			float vignetteWidth;        ///< Fisheye only.  Distance from the center of the viewport to the center of the vignette.
			float vignetteSharpness;    ///< Fisheye only.  Distance that the vignette gradient covers.
		} camera;

		struct
		{
			VuoColor color;
			float brightness;
			float range;      ///< Distance (in local coordinates) the light reaches.  Affects point lights and spotlights.
			float cone;       ///< Size (in radians) of the light's cone.  Affects spotlights.
			float sharpness;  ///< Sharpness of the light's distance/cone falloff.  0 means the light starts fading at distance/angle 0 and ends at 2*lightRange or 2*lightCone.  1 means the falloff is instant.
		} light;

		struct
		{
			VuoText text;
			VuoFont font;
			bool scaleWithScene;
			float wrapWidth;
		} text;
	};
} VuoSceneObject_internal;

/**
 * Returns a number, unique within this process,
 * for identifying a particlar scene object instance.
 */
uint64_t VuoSceneObject_getNextId(void)
{
	static uint64_t id = 0;
	return __sync_add_and_fetch(&id, 1);
}

/**
 * Frees the memory associated with the object.
 *
 * @threadAny
 */
void VuoSceneObject_free(void *sceneObject)
{
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;

	VuoRelease(so->name);
	VuoRelease(so->mesh);
	VuoRelease(so->shader);
	if (so->type == VuoSceneObjectSubType_Group)
		VuoRelease(so->childObjects);
	else if (so->type == VuoSceneObjectSubType_Text)
	{
		VuoRelease(so->text.text);
		VuoFont_release(so->text.font);
	}

	free(so);
}

/**
 * Creates a new, empty scene object.
 */
VuoSceneObject VuoSceneObject_makeEmpty(void)
{
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)calloc(1, sizeof(VuoSceneObject_internal));
	VuoRegister(so, VuoSceneObject_free);

	so->id = 0;
	so->type = VuoSceneObjectSubType_Empty;

	so->mesh = NULL;
	so->shader = NULL;
	so->isRealSize = false;
	so->preservePhysicalSize = false;
	so->blendMode = VuoBlendMode_Normal;

	so->name = NULL;
	so->transform = VuoTransform_makeIdentity();

	return (VuoSceneObject)so;
}

/**
 * Creates a new scene object that can contain (and transform) other scene objects, but doesn't render anything itself.
 */
VuoSceneObject VuoSceneObject_makeGroup(VuoList_VuoSceneObject childObjects, VuoTransform transform)
{
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)VuoSceneObject_makeEmpty();

	so->type = VuoSceneObjectSubType_Group;

	VuoSceneObject_setChildObjects((VuoSceneObject)so, childObjects);

	so->transform = transform;

	return (VuoSceneObject)so;
}

/**
 * Creates a visible (mesh) scene object.
 */
VuoSceneObject VuoSceneObject_makeMesh(VuoMesh mesh, VuoShader shader, VuoTransform transform)
{
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)VuoSceneObject_makeEmpty();

	so->type = VuoSceneObjectSubType_Mesh;

	VuoSceneObject_setMesh((VuoSceneObject)so, mesh);

	if (mesh && !shader)
		VuoSceneObject_setShader((VuoSceneObject)so, VuoShader_makeDefaultShader());
	else
		VuoSceneObject_setShader((VuoSceneObject)so, shader);

	so->transform = transform;

	return (VuoSceneObject)so;
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
	return VuoSceneObject_makeMesh(
				VuoMesh_makeQuadWithoutNormals(),
				shader,
				VuoTransform_makeEuler(
					center,
					VuoPoint3d_multiply(rotation, M_PI/180.),
					VuoPoint3d_make(width,height,1)
				));
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
	return VuoSceneObject_makeMesh(
				VuoMesh_makeQuad(),
				shader,
				VuoTransform_makeEuler(
					center,
					VuoPoint3d_multiply(rotation, M_PI/180.),
					VuoPoint3d_make(width,height,1)
				));
}

/**
 * Returns an unlit scene object with the specified @c image.
 *
 * @threadAnyGL
 */
VuoSceneObject VuoSceneObject_makeImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal size, VuoOrientation fixed, VuoReal alpha)
{
	if (!image)
		return nullptr;

	float width, height;
	if (fixed == VuoOrientation_Horizontal)
	{
		width = size;
		height = image->pixelsHigh * size / image->pixelsWide;
	}
	else
	{
		width = image->pixelsWide * size / image->pixelsHigh;
		height = size;
	}

	VuoSceneObject object = VuoSceneObject_makeQuad(
				VuoShader_makeUnlitImageShader(image, alpha),
				center,
				rotation,
				width,
				height
			);

	return object;
}

/**
 * Returns a lit scene object with the specified @c image.
 *
 * @threadAnyGL
 */
VuoSceneObject VuoSceneObject_makeLitImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal size, VuoOrientation fixed, VuoReal alpha, VuoColor highlightColor, VuoReal shininess)
{
	if (!image)
		return nullptr;

	float width, height;
	if (fixed == VuoOrientation_Horizontal)
	{
		width = size;
		height = image->pixelsHigh * size / image->pixelsWide;
	}
	else
	{
		width = image->pixelsWide * size / image->pixelsHigh;
		height = size;
	}

	return VuoSceneObject_makeQuadWithNormals(
				VuoShader_makeLitImageShader(image, alpha, highlightColor, shininess),
				center,
				rotation,
				width,
				height
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
		VuoSceneObject so = VuoSceneObject_makeMesh(
			quadMesh,
			frontShader,
			VuoTransform_makeEuler(VuoPoint3d_make(0,0,.5), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)));
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Left Face
	{
		VuoSceneObject so = VuoSceneObject_makeMesh(
			quadMesh,
			leftShader,
			VuoTransform_makeEuler(VuoPoint3d_make(-.5,0,0), VuoPoint3d_make(0,-M_PI/2.,0), VuoPoint3d_make(1,1,1)));
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Right Face
	{
		VuoSceneObject so = VuoSceneObject_makeMesh(
			quadMesh,
			rightShader,
			VuoTransform_makeEuler(VuoPoint3d_make(.5,0,0), VuoPoint3d_make(0,M_PI/2.,0), VuoPoint3d_make(1,1,1)));
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Back Face
	{
		VuoSceneObject so = VuoSceneObject_makeMesh(
			quadMesh,
			backShader,
			VuoTransform_makeEuler(VuoPoint3d_make(0,0,-.5), VuoPoint3d_make(0,M_PI,0), VuoPoint3d_make(1,1,1)));
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Top Face
	{
		VuoSceneObject so = VuoSceneObject_makeMesh(
			quadMesh,
			topShader,
			VuoTransform_makeEuler(VuoPoint3d_make(0,.5,0), VuoPoint3d_make(-M_PI/2.,0,0), VuoPoint3d_make(1,1,1)));
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Bottom Face
	{
		VuoSceneObject so = VuoSceneObject_makeMesh(
			quadMesh,
			bottomShader,
			VuoTransform_makeEuler(VuoPoint3d_make(0,-.5,0), VuoPoint3d_make(M_PI/2.,0,0), VuoPoint3d_make(1,1,1)));
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	return VuoSceneObject_makeGroup(cubeChildObjects, transform);
}


/**
 * Returns a cube scene object with a single shader applied to all 6 sides.
 *
 * @deprecated{Use `VuoSceneObject_makeCube_Vuo*()` instead.}
 */
VuoSceneObject VuoSceneObject_makeCube1(VuoTransform transform, VuoShader shader)
{
    return VuoSceneObject_makeMesh(VuoMesh_makeCube(), shader, transform);
}

/**
 * Creates a cube painted on all sides by `shader`.
 *
 * @version200New
 */
VuoSceneObject VuoSceneObject_makeCube_VuoShader(VuoTransform transform, VuoShader shader)
{
    return VuoSceneObject_makeMesh(VuoMesh_makeCube(), shader, transform);
}

/**
 * Creates a cube painted on all sides by `image`.
 *
 * @version200New
 */
VuoSceneObject VuoSceneObject_makeCube_VuoImage(VuoTransform transform, VuoImage image)
{
    return VuoSceneObject_makeMesh(VuoMesh_makeCube(), VuoShader_make_VuoImage(image), transform);
}

/**
 * Creates a cube painted on all sides by `color`.
 *
 * @version200New
 */
VuoSceneObject VuoSceneObject_makeCube_VuoColor(VuoTransform transform, VuoColor color)
{
    return VuoSceneObject_makeMesh(VuoMesh_makeCube(), VuoShader_make_VuoColor(color), transform);
}

/**
 * Creates a cube painted with a cubemap.
 *
 * @version200New
 */
VuoSceneObject VuoSceneObject_makeCube_VuoCubemap(VuoTransform transform, VuoCubemap cubemap)
{
    return VuoSceneObject_makeCubeMulti(transform, 2, 2, 2,
        VuoShader_make_VuoImage(VuoCubemap_getFront(cubemap)),
        VuoShader_make_VuoImage(VuoCubemap_getLeft(cubemap)),
        VuoShader_make_VuoImage(VuoCubemap_getRight(cubemap)),
        VuoShader_make_VuoImage(VuoCubemap_getBack(cubemap)),
        VuoShader_make_VuoImage(VuoCubemap_getTop(cubemap)),
        VuoShader_make_VuoImage(VuoCubemap_getBottom(cubemap)));
}

/**
 * Creates a cube with subdivided faces and multiple shaders.
 *
 * @version200New
 */
VuoSceneObject VuoSceneObject_makeCubeMulti(VuoTransform transform, VuoInteger columns, VuoInteger rows, VuoInteger slices,
    VuoShader front, VuoShader left, VuoShader right, VuoShader back, VuoShader top, VuoShader bottom)
{
    VuoList_VuoSceneObject cubeChildObjects = VuoListCreate_VuoSceneObject();

	unsigned int _rows = MAX(2, MIN(512, rows));
	unsigned int _columns = MAX(2, MIN(512, columns));
	unsigned int _slices = MAX(2, MIN(512, slices));

	VuoMesh frontBackMesh = VuoMesh_makePlane(_columns, _rows);
	VuoMesh leftRightMesh = VuoMesh_makePlane(_slices, _rows);
	VuoMesh topBottomMesh = VuoMesh_makePlane(_columns, _slices);

	// Front Face
	{
		VuoSceneObject so = VuoSceneObject_makeMesh(
			frontBackMesh,
			front,
			VuoTransform_makeEuler(VuoPoint3d_make(0,0,.5), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)));
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Left Face
	{
		VuoSceneObject so = VuoSceneObject_makeMesh(
			leftRightMesh,
			left,
			VuoTransform_makeEuler(VuoPoint3d_make(-.5,0,0), VuoPoint3d_make(0,-M_PI/2.,0), VuoPoint3d_make(1,1,1)));
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Right Face
	{
		VuoSceneObject so = VuoSceneObject_makeMesh(
			leftRightMesh,
			right,
			VuoTransform_makeEuler(VuoPoint3d_make(.5,0,0), VuoPoint3d_make(0,M_PI/2.,0), VuoPoint3d_make(1,1,1)));
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Back Face
	{
		VuoSceneObject so = VuoSceneObject_makeMesh(
			frontBackMesh,
			back,
			VuoTransform_makeEuler(VuoPoint3d_make(0,0,-.5), VuoPoint3d_make(0,M_PI,0), VuoPoint3d_make(1,1,1)));
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Top Face
	{
		VuoSceneObject so = VuoSceneObject_makeMesh(
			topBottomMesh,
			top,
			VuoTransform_makeEuler(VuoPoint3d_make(0,.5,0), VuoPoint3d_make(-M_PI/2.,0,0), VuoPoint3d_make(1,1,1)));
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Bottom Face
	{
		VuoSceneObject so = VuoSceneObject_makeMesh(
			topBottomMesh,
			bottom,
			VuoTransform_makeEuler(VuoPoint3d_make(0,-.5,0), VuoPoint3d_make(M_PI/2.,0,0), VuoPoint3d_make(1,1,1)));
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	return VuoSceneObject_makeGroup(cubeChildObjects, transform);
}

/**
 * Returns a scene object representing deferred-rendered text.
 *
 * The caller is responsible for providing the sceneobject's mesh (e.g., @ref VuoMesh_makeQuadWithoutNormals).
 *
 * @threadAny
 * @version200Changed{Added `scaleWithScene`, `wrapWidth` arguments.}
 */
VuoSceneObject VuoSceneObject_makeText(VuoText text, VuoFont font, VuoBoolean scaleWithScene, float wrapWidth)
{
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)VuoSceneObject_makeEmpty();
	so->type = VuoSceneObjectSubType_Text;
	VuoSceneObject_setText((VuoSceneObject)so, text);
	VuoSceneObject_setTextFont((VuoSceneObject)so, font);
	so->text.scaleWithScene = scaleWithScene;
	so->text.wrapWidth = wrapWidth;
	return (VuoSceneObject)so;
}

/**
 * Returns a perspective camera having the position and negative-rotation specified by @c transform (its scale is ignored).
 */
VuoSceneObject VuoSceneObject_makePerspectiveCamera(VuoText name, VuoTransform transform, float fieldOfView, float distanceMin, float distanceMax)
{
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)VuoSceneObject_makeEmpty();
	so->type = VuoSceneObjectSubType_PerspectiveCamera;
	VuoSceneObject_setName((VuoSceneObject)so, name);
	so->transform = transform;
	so->camera.fieldOfView = fieldOfView;
	so->camera.distanceMin = distanceMin;
	so->camera.distanceMax = distanceMax;
	return (VuoSceneObject)so;
}

/**
 * Returns a stereoscopic camera having the position and negative-rotation specified by @c transform (its scale is ignored).
 */
VuoSceneObject VuoSceneObject_makeStereoCamera(VuoText name, VuoTransform transform, VuoReal fieldOfView, VuoReal distanceMin, VuoReal distanceMax, VuoReal confocalDistance, VuoReal intraocularDistance)
{
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)VuoSceneObject_makeEmpty();
	so->type = VuoSceneObjectSubType_StereoCamera;
	VuoSceneObject_setName((VuoSceneObject)so, name);
	so->transform = transform;
	so->camera.fieldOfView = fieldOfView;
	so->camera.distanceMin = distanceMin;
	so->camera.distanceMax = distanceMax;
	so->camera.confocalDistance = confocalDistance;
	so->camera.intraocularDistance = intraocularDistance;
	return (VuoSceneObject)so;
}

/**
 * Returns an orthographic camera having the position and negative-rotation specified by @c transform (its scale is ignored).
 */
VuoSceneObject VuoSceneObject_makeOrthographicCamera(VuoText name, VuoTransform transform, float width, float distanceMin, float distanceMax)
{
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)VuoSceneObject_makeEmpty();
	so->type = VuoSceneObjectSubType_OrthographicCamera;
	VuoSceneObject_setName((VuoSceneObject)so, name);
	so->transform = transform;
	so->camera.width = width;
	so->camera.distanceMin = distanceMin;
	so->camera.distanceMax = distanceMax;
	return (VuoSceneObject)so;
}

/**
 * Returns a fisheye camera having the position and negative-rotation specified by @c transform (its scale is ignored).
 */
VuoSceneObject VuoSceneObject_makeFisheyeCamera(VuoText name, VuoTransform transform, VuoReal fieldOfView, VuoReal vignetteWidth, VuoReal vignetteSharpness)
{
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)VuoSceneObject_makeEmpty();
	so->type = VuoSceneObjectSubType_FisheyeCamera;
	VuoSceneObject_setName((VuoSceneObject)so, name);
	so->transform = transform;
	so->camera.fieldOfView = fieldOfView;

	// 0 and 1000 come from "Realtime Dome Imaging and Interaction" by Bailey/Clothier/Gebbie 2006.
	so->camera.distanceMin = 0;
	so->camera.distanceMax = 1000;

	so->camera.vignetteWidth = vignetteWidth;
	so->camera.vignetteSharpness = vignetteSharpness;

	return (VuoSceneObject)so;
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
 * @param sceneObject The root object of the scenegraph to search.
 * @param nameToMatch The name to search for.
 * @param[out] ancestorObjects The ancestors of @a foundObject, starting with the root of the scenegraph.
 * @param[out] foundObject The first matching scene object found.
 * @return True if a matching scene object was found.
 */
bool VuoSceneObject_find(VuoSceneObject sceneObject, VuoText nameToMatch, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject *foundObject)
{
	if (!sceneObject)
		return false;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;

	if (VuoText_areEqual(so->name, nameToMatch))
	{
		*foundObject = (VuoSceneObject)so;
		return true;
	}

	if (so->type == VuoSceneObjectSubType_Group)
	{
		VuoListAppendValue_VuoSceneObject(ancestorObjects, sceneObject);

		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so->childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject childObject = VuoListGetValue_VuoSceneObject(so->childObjects, i);
			if (VuoSceneObject_find(childObject, nameToMatch, ancestorObjects, foundObject))
				return true;
		}

		VuoListRemoveLastValue_VuoSceneObject(ancestorObjects);
	}

	return false;
}

/**
 * Searches the scenegraph (depth-first) for a scene object with the given id.
 *
 * @param sceneObject The root object of the scenegraph to search.
 * @param idToMatch The id to search for.
 * @param[out] ancestorObjects The ancestors of @a foundObject, starting with the root of the scenegraph.
 * @param[out] foundObject The first matching scene object found.
 * @return True if a matching scene object was found.
 * @version200New
 */
bool VuoSceneObject_findById(VuoSceneObject sceneObject, uint64_t idToMatch, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject *foundObject)
{
	if (!sceneObject)
		return false;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;

	if (so->id == idToMatch)
	{
		*foundObject = (VuoSceneObject)so;
		return true;
	}

	if (so->type == VuoSceneObjectSubType_Group)
	{
		VuoListAppendValue_VuoSceneObject(ancestorObjects, sceneObject);

		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so->childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject childObject = VuoListGetValue_VuoSceneObject(so->childObjects, i);
			if (VuoSceneObject_findById(childObject, idToMatch, ancestorObjects, foundObject))
				return true;
		}

		VuoListRemoveLastValue_VuoSceneObject(ancestorObjects);
	}

	return false;
}

/**
 * Searches the scenegraph (depth-first) for the first scene object with the specified type.
 *
 * @param sceneObject The root object of the scenegraph to search.
 * @param typeToMatch The sub-type to search for.
 * @param[out] ancestorObjects The ancestors of @a foundObject, starting with the root of the scenegraph.
 * @param[out] foundObject The first matching scene object found.
 * @return True if a matching scene object was found.
 * @version200New
 */
bool VuoSceneObject_findWithType(VuoSceneObject sceneObject, VuoSceneObjectSubType typeToMatch, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject *foundObject)
{
	if (!sceneObject)
		return false;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;

	if (so->type == typeToMatch)
	{
		*foundObject = (VuoSceneObject)so;
		return true;
	}

	if (so->type == VuoSceneObjectSubType_Group)
	{
		VuoListAppendValue_VuoSceneObject(ancestorObjects, sceneObject);

		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so->childObjects);

		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject childObject = VuoListGetValue_VuoSceneObject(so->childObjects, i);

			if (VuoSceneObject_findWithType(childObject, typeToMatch, ancestorObjects, foundObject))
				return true;
		}

		VuoListRemoveLastValue_VuoSceneObject(ancestorObjects);
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
bool VuoSceneObject_findCamera(VuoSceneObject sceneObject, VuoText nameToMatch, VuoSceneObject *foundCamera)
{
	if (!sceneObject)
		return false;

	__block bool didFindCamera = false;
	VuoSceneObject_visit(sceneObject, ^(const VuoSceneObject currentObject, float modelviewMatrix[16]){
		VuoSceneObject_internal *co = (VuoSceneObject_internal *)currentObject;
		if ((co->type == VuoSceneObjectSubType_PerspectiveCamera
		  || co->type == VuoSceneObjectSubType_StereoCamera
		  || co->type == VuoSceneObjectSubType_OrthographicCamera
		  || co->type == VuoSceneObjectSubType_FisheyeCamera)
		  && (!nameToMatch || (co->name && nameToMatch && strstr(co->name, nameToMatch))))
		{
			*foundCamera = currentObject;
			VuoSceneObject_setTransform(*foundCamera, VuoTransform_makeFromMatrix4x4(modelviewMatrix));
			didFindCamera = true;
			return false;
		}
		return true;
	});

	return didFindCamera;
}

/**
 * Returns true if the scene object has a non-empty type.
 */
bool VuoSceneObject_isPopulated(VuoSceneObject sceneObject)
{
	if (!sceneObject)
		return false;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;
	return so->type != VuoSceneObjectSubType_Empty;
}

/**
 * Returns the `VuoSceneObjectSubType` corresponding with the `typeString`.  If none matches, returns VuoSceneObjectSubType_Empty.
 */
static VuoSceneObjectSubType VuoSceneObject_typeFromCString(const char *typeString)
{
	if (strcmp(typeString,"empty")==0)
		return VuoSceneObjectSubType_Empty;
	else if (strcmp(typeString,"group")==0)
		return VuoSceneObjectSubType_Group;
	else if (strcmp(typeString,"mesh")==0)
		return VuoSceneObjectSubType_Mesh;
	else if (strcmp(typeString,"camera-perspective")==0)
		return VuoSceneObjectSubType_PerspectiveCamera;
	else if (strcmp(typeString,"camera-stereo")==0)
		return VuoSceneObjectSubType_StereoCamera;
	else if (strcmp(typeString,"camera-orthographic")==0)
		return VuoSceneObjectSubType_OrthographicCamera;
	else if (strcmp(typeString,"camera-fisheye")==0)
		return VuoSceneObjectSubType_FisheyeCamera;
	else if (strcmp(typeString,"light-ambient")==0)
		return VuoSceneObjectSubType_AmbientLight;
	else if (strcmp(typeString,"light-point")==0)
		return VuoSceneObjectSubType_PointLight;
	else if (strcmp(typeString,"light-spot")==0)
		return VuoSceneObjectSubType_Spotlight;
	else if (strcmp(typeString,"text")==0)
		return VuoSceneObjectSubType_Text;

	return VuoSceneObjectSubType_Empty;
}

/**
 * Returns a string constant representing `type`.
 */
static const char * VuoSceneObject_cStringForType(VuoSceneObjectSubType type)
{
	switch (type)
	{
		case VuoSceneObjectSubType_Group:
			return "group";
		case VuoSceneObjectSubType_Mesh:
			return "mesh";
		case VuoSceneObjectSubType_PerspectiveCamera:
			return "camera-perspective";
		case VuoSceneObjectSubType_StereoCamera:
			return "camera-stereo";
		case VuoSceneObjectSubType_OrthographicCamera:
			return "camera-orthographic";
		case VuoSceneObjectSubType_FisheyeCamera:
			return "camera-fisheye";
		case VuoSceneObjectSubType_AmbientLight:
			return "light-ambient";
		case VuoSceneObjectSubType_PointLight:
			return "light-point";
		case VuoSceneObjectSubType_Spotlight:
			return "light-spot";
		case VuoSceneObjectSubType_Text:
			return "text";
		// VuoSceneObjectSubType_Empty
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
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)VuoSceneObject_makeEmpty();
	so->type = VuoSceneObjectSubType_AmbientLight;
	VuoSceneObject_setName((VuoSceneObject)so, VuoText_make("Ambient Light"));
	so->light.color = color;
	so->light.brightness = brightness;
	return (VuoSceneObject)so;
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
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)VuoSceneObject_makeEmpty();
	so->type = VuoSceneObjectSubType_PointLight;
	VuoText t = VuoText_make("Point Light");
	VuoSceneObject_setName((VuoSceneObject)so, t);
	so->light.color = color;
	so->light.brightness = brightness;
	so->light.range = range;
	so->light.sharpness = MAX(MIN(sharpness,1),0);
	so->transform = VuoTransform_makeEuler(position, VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1));
	return (VuoSceneObject)so;
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
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)VuoSceneObject_makeEmpty();
	so->type = VuoSceneObjectSubType_Spotlight;
	VuoSceneObject_setName((VuoSceneObject)so, VuoText_make("Spot Light"));
	so->light.color = color;
	so->light.brightness = brightness;
	so->light.cone = cone;
	so->light.range = range;
	so->light.sharpness = MAX(MIN(sharpness,1),0);
	so->transform = transform;
	return (VuoSceneObject)so;
}

/**
 * Finds and returns all the lights in the scene,
 * with their transforms applied.
 *
 * If there are multiple ambient lights, returns the weighted (by alpha) average color and summed brightness.
 *
 * If there are no lights in the scene, returns some default lights.
 */
void VuoSceneObject_findLights(VuoSceneObject sceneObject, VuoColor *ambientColor, float *ambientBrightness, VuoList_VuoSceneObject *pointLights, VuoList_VuoSceneObject *spotLights)
{
	__block VuoList_VuoColor ambientColors = VuoListCreate_VuoColor();
	VuoRetain(ambientColors);

	*ambientBrightness = 0;
	*pointLights = VuoListCreate_VuoSceneObject();
	*spotLights = VuoListCreate_VuoSceneObject();

	VuoSceneObject_visit(sceneObject, ^(const VuoSceneObject currentObject, float modelviewMatrix[16]){
		VuoSceneObject_internal *co = (VuoSceneObject_internal *)currentObject;
		if (co->type == VuoSceneObjectSubType_AmbientLight)
		{
			VuoListAppendValue_VuoColor(ambientColors, co->light.color);
			*ambientBrightness += co->light.brightness;
		}
		else if (co->type == VuoSceneObjectSubType_PointLight)
		{
			VuoSceneObject l = VuoSceneObject_copy((VuoSceneObject)co);
			VuoSceneObject_setTransform(l, VuoTransform_makeFromMatrix4x4(modelviewMatrix));
			VuoListAppendValue_VuoSceneObject(*pointLights, l);
		}
		else if (co->type == VuoSceneObjectSubType_Spotlight)
		{
			VuoSceneObject l = VuoSceneObject_copy((VuoSceneObject)co);
			VuoSceneObject_setTransform(l, VuoTransform_makeFromMatrix4x4(modelviewMatrix));
			VuoListAppendValue_VuoSceneObject(*spotLights, l);
		}
		return true;
	});

	if (!VuoListGetCount_VuoColor(ambientColors)
			&& !VuoListGetCount_VuoSceneObject(*pointLights)
			&& !VuoListGetCount_VuoSceneObject(*spotLights))
	{
		*ambientColor = VuoColor_makeWithRGBA(1,1,1,1);
		*ambientBrightness = 0.05;

		// https://en.wikipedia.org/wiki/Three-point_lighting

		VuoSceneObject keyLight  = VuoSceneObject_makePointLight(VuoColor_makeWithRGBA(1,1,1,1), .70, VuoPoint3d_make(-1,1,1), 5, .5);
		VuoListAppendValue_VuoSceneObject(*pointLights, keyLight);

		VuoSceneObject fillLight = VuoSceneObject_makePointLight(VuoColor_makeWithRGBA(1,1,1,1), .20, VuoPoint3d_make(.5,0,1), 5, 0);
		VuoListAppendValue_VuoSceneObject(*pointLights, fillLight);

		VuoSceneObject backLight = VuoSceneObject_makePointLight(VuoColor_makeWithRGBA(1,1,1,1), .15, VuoPoint3d_make(1,.75,-.5), 5, 0);
		VuoListAppendValue_VuoSceneObject(*pointLights, backLight);
	}
	else
		*ambientColor = VuoColor_average(ambientColors);

	VuoRelease(ambientColors);
}

/**
 * State for visiting an object at its particular place in the hierarchy.
 */
typedef struct
{
	long objectCount;
	const VuoSceneObject *objects;
	float modelviewMatrix[16];
} VuoSceneObject_treeState;

/**
 * Applies `function` to `object` and its child objects,
 * without preserving changes to objects.
 *
 * If `function` returns false, visiting will stop immediately
 * (possibly before all objects have been visited).
 *
 * The value `modelviewMatrix` (which `VuoSceneObject_visit` passes to `function`)
 * is the cumulative transformation matrix (from `object` down to the `currentObject`).
 *
 * NULL objects in the tree are ignored (`function` is not called).
 */
void VuoSceneObject_visit(const VuoSceneObject object, bool (^function)(const VuoSceneObject currentObject, float modelviewMatrix[16]))
{
	if (!object)
		return;

	VuoSceneObject_treeState rootState;
	rootState.objectCount = 1;
	rootState.objects = &object;
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), rootState.modelviewMatrix);

	std::list<VuoSceneObject_treeState> objectsToVisit(1, rootState);
	while (!objectsToVisit.empty())
	{
		VuoSceneObject_treeState currentState = objectsToVisit.front();
		objectsToVisit.pop_front();

		for (long i = 0; i < currentState.objectCount; ++i)
		{
			VuoSceneObject_internal *currentObject = (VuoSceneObject_internal *)currentState.objects[i];
			if (!currentObject)
				continue;

			float localModelviewMatrix[16];
			VuoTransform_getMatrix(currentObject->transform, localModelviewMatrix);
			float compositeModelviewMatrix[16];
			VuoTransform_multiplyMatrices4x4(localModelviewMatrix, currentState.modelviewMatrix, compositeModelviewMatrix);

			if (!function((VuoSceneObject)currentObject, compositeModelviewMatrix))
				return;

			if (currentObject->type == VuoSceneObjectSubType_Group)
			{
				// Prepend this object's childObjects to the objectsToVisit queue.
				long childObjectCount = VuoListGetCount_VuoSceneObject(currentObject->childObjects);
				if (childObjectCount)
				{
					VuoSceneObject_treeState childState;
					childState.objectCount = childObjectCount;
					childState.objects = VuoListGetData_VuoSceneObject(currentObject->childObjects);
					memcpy(childState.modelviewMatrix, compositeModelviewMatrix, sizeof(float[16]));
					objectsToVisit.push_front(childState);
				}
			}
		}
	}
}

/**
 * Helper for @ref VuoSceneObject_apply.
 */
static void VuoSceneObject_applyInternal(VuoSceneObject sceneObject, void (^function)(VuoSceneObject currentObject, float modelviewMatrix[16]), float modelviewMatrix[16])
{
	if (!sceneObject)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;

	float localModelviewMatrix[16];
	VuoTransform_getMatrix(so->transform, localModelviewMatrix);
	float compositeModelviewMatrix[16];
	VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);

	function(sceneObject, compositeModelviewMatrix);

	if (so->type == VuoSceneObjectSubType_Group && so->childObjects)
	{
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so->childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject o = VuoListGetValue_VuoSceneObject(so->childObjects, i);
			VuoSceneObject_applyInternal(o, function, compositeModelviewMatrix);
			VuoListSetValue_VuoSceneObject(so->childObjects, o, i, false);
		}
	}
}

/**
 * Applies @c function to @c object and its child objects,
 * and outputs the modified @c object.
 *
 * The value `modelviewMatrix` (which `VuoSceneObject_apply` passes to `function`)
 * is the cumulative transformation matrix (from `object` down to the `currentObject`).
 *
 * NULL objects in the tree are ignored (`function` is not called).
 */
void VuoSceneObject_apply(VuoSceneObject object, void (^function)(VuoSceneObject currentObject, float modelviewMatrix[16]))
{
	if (!object)
		return;

	float localModelviewMatrix[16];
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), localModelviewMatrix);

	VuoSceneObject_applyInternal(object, function, localModelviewMatrix);
}

/**
 * Applies a transformation to the sceneobject (combining it with its previous transform).
 *
 * @version200New
 */
void VuoSceneObject_transform(VuoSceneObject object, VuoTransform transform)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->transform = VuoTransform_composite(so->transform, transform);
}

/**
 * Moves the sceneobject in 3D space.
 *
 * @version200New
 */
void VuoSceneObject_translate(VuoSceneObject object, VuoPoint3d translation)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->transform.translation += translation;
}

/**
 * Scales the sceneobject in 3D space.
 *
 * @version200New
 */
void VuoSceneObject_scale(VuoSceneObject object, VuoPoint3d scale)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->transform.scale *= scale;
}

/**
 * Returns the sceneobject's display name.
 *
 * @version200New
 */
VuoText VuoSceneObject_getName(VuoSceneObject object)
{
	if (!object)
		return nullptr;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->name;
}

/**
 * Returns the list of this sceneobject's child sceneobjects.
 *
 * The caller is permitted to modify the returned value (e.g., append items to the list),
 * which will affect this sceneobject.
 *
 * @version200New
 */
VuoList_VuoSceneObject VuoSceneObject_getChildObjects(VuoSceneObject object)
{
	if (!object)
		return nullptr;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	if (so->type != VuoSceneObjectSubType_Group)
		return nullptr;

	return so->childObjects;
}

/**
 * Returns the sceneobject's blend mode.
 *
 * @version200New
 */
VuoBlendMode VuoSceneObject_getBlendMode(const VuoSceneObject object)
{
	if (!object)
		return VuoBlendMode_Normal;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->blendMode;
}

/**
 * Returns the sceneobject's type.
 *
 * @version200New
 */
VuoSceneObjectSubType VuoSceneObject_getType(const VuoSceneObject object)
{
	if (!object)
		return VuoSceneObjectSubType_Empty;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->type;
}

/**
 * Returns the sceneobject's identification number
 * (unique among objects in the currently-running composition).
 *
 * @version200New
 */
uint64_t VuoSceneObject_getId(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->id;
}

/**
 * Returns the sceneobject's shader.
 *
 * The caller is permitted to modify the returned value (e.g., change the shader's uniforms),
 * which will affect this sceneobject.
 *
 * @version200New
 */
VuoShader VuoSceneObject_getShader(const VuoSceneObject object)
{
	if (!object)
		return nullptr;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->shader;
}

/**
 * Returns true if the sceneobject should ignore rotations and scales,
 * and be sized to match the shader's first image.
 *
 * @version200New
 */
bool VuoSceneObject_isRealSize(const VuoSceneObject object)
{
	if (!object)
		return false;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->isRealSize;
}

/**
 * Returns true if the sceneobject should use the texture's scaleFactor
 * and the backingScaleFactor to determine the rendered size.
 * Returns false if the texture should always be rendered 1:1.
 *
 * @version200New
 */
bool VuoSceneObject_shouldPreservePhysicalSize(const VuoSceneObject object)
{
	if (!object)
		return false;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->preservePhysicalSize;
}

/**
 * Returns the sceneobject's mesh.
 *
 * The caller is permitted to modify the returned value (e.g., change the mesh's buffers),
 * which will affect this sceneobject.
 *
 * @version200New
 */
VuoMesh VuoSceneObject_getMesh(const VuoSceneObject object)
{
	if (!object)
		return nullptr;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->mesh;
}

/**
 * Returns the sceneobject's rendered text.
 *
 * @version200New
 */
VuoText VuoSceneObject_getText(const VuoSceneObject object)
{
	if (!object)
		return nullptr;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->text.text;
}

/**
 * Returns the sceneobject's font.
 *
 * @version200New
 */
VuoFont VuoSceneObject_getTextFont(const VuoSceneObject object)
{
	if (!object)
		return VuoFont_makeDefault();

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->text.font;
}

/**
 * Returns true if the sceneobject's text
 * should change depending on the scene's rendering destination,
 * or false if it should maintain its nominal size.
 *
 * @version200New
 */
bool VuoSceneObject_shouldTextScaleWithScene(const VuoSceneObject object)
{
	if (!object)
		return false;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->text.scaleWithScene;
}

/**
 * Returns the width at which the sceneobject's rendered text should wrap.
 *
 * @version200New
 */
float VuoSceneObject_getTextWrapWidth(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->text.wrapWidth;
}

/**
 * Returns the sceneobject's field of view (for perspective cameras).
 *
 * @version200New
 */
float VuoSceneObject_getCameraFieldOfView(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->camera.fieldOfView;
}

/**
 * Returns the sceneobject's camera width (for isometric cameras).
 *
 * @version200New
 */
float VuoSceneObject_getCameraWidth(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->camera.width;
}

/**
 * Returns the sceneobject's depth buffer minimum distance.
 *
 * @version200New
 */
float VuoSceneObject_getCameraDistanceMin(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->camera.distanceMin;
}

/**
 * Returns the sceneobject's depth buffer maximum distance.
 *
 * @version200New
 */
float VuoSceneObject_getCameraDistanceMax(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->camera.distanceMax;
}

/**
 * Returns the sceneobject's vignette width (for fisheye cameras).
 *
 * @version200New
 */
float VuoSceneObject_getCameraVignetteWidth(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->camera.vignetteWidth;
}

/**
 * Returns the sceneobject's vignette sharpness (for fisheye cameras).
 *
 * @version200New
 */
float VuoSceneObject_getCameraVignetteSharpness(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->camera.vignetteSharpness;
}

/**
 * Returns the sceneobject's camera intraocular distance (for stereoscopic cameras).
 *
 * @version200New
 */
float VuoSceneObject_getCameraIntraocularDistance(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->camera.intraocularDistance;
}

/**
 * Returns the sceneobject's camera confocal distance (for stereoscopic cameras).
 *
 * @version200New
 */
float VuoSceneObject_getCameraConfocalDistance(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->camera.confocalDistance;
}

/**
 * Returns the sceneobject's light color.
 *
 * @version200New
 */
VuoColor VuoSceneObject_getLightColor(const VuoSceneObject object)
{
	if (!object)
		return (VuoColor){0,0,0,0};

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->light.color;
}

/**
 * Returns the sceneobject's light brightness.
 *
 * @version200New
 */
float VuoSceneObject_getLightBrightness(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->light.brightness;
}

/**
 * Returns the sceneobject's light cone range.
 *
 * @version200New
 */
float VuoSceneObject_getLightRange(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->light.range;
}

/**
 * Returns the sceneobject's light cone sharpness.
 *
 * @version200New
 */
float VuoSceneObject_getLightSharpness(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->light.sharpness;
}

/**
 * Returns the sceneobject's light cone angle.
 *
 * @version200New
 */
float VuoSceneObject_getLightCone(const VuoSceneObject object)
{
	if (!object)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->light.cone;
}

/**
 * Returns the sceneobject's transform.
 *
 * @version200New
 */
VuoTransform VuoSceneObject_getTransform(const VuoSceneObject object)
{
	if (!object)
		return VuoTransform_makeIdentity();

   VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
   return so->transform;
}

/**
 * Returns the sceneobject's transform's translation.
 *
 * @version200New
 */
VuoPoint3d VuoSceneObject_getTranslation(const VuoSceneObject object)
{
	if (!object)
		return (VuoPoint3d){0,0,0};

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	return so->transform.translation;
}

/**
 * Changes the sceneobject's type.
 *
 * @version200New
 */
void VuoSceneObject_setType(VuoSceneObject object, VuoSceneObjectSubType type)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->type = type;
}

/**
 * Changes the sceneobject's identification number
 * (should be unique among objects in the currently-running composition).
 *
 * @version200New
 */
void VuoSceneObject_setId(VuoSceneObject object, uint64_t id)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->id = id;
}

/**
 * Changes the sceneobject's display name.
 *
 * @version200New
 */
void VuoSceneObject_setName(VuoSceneObject object, VuoText name)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	VuoRetain(name);
	VuoRelease(so->name);
	so->name = name;
}

/**
 * Changes the sceneobject's list of child objects.
 *
 * @version200New
 */
void VuoSceneObject_setChildObjects(VuoSceneObject object, VuoList_VuoSceneObject childObjects)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	if (so->type != VuoSceneObjectSubType_Group)
		return;

	VuoRetain(childObjects);
	VuoRelease(so->childObjects);
	so->childObjects = childObjects;
}

/**
 * Changes the sceneobject's mesh.
 *
 * @version200New
 */
void VuoSceneObject_setMesh(VuoSceneObject object, VuoMesh mesh)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	VuoRetain(mesh);
	VuoRelease(so->mesh);
	so->mesh = mesh;
}

/**
 * Changes the sceneobject's transform.
 *
 * @version200New
 */
void VuoSceneObject_setTransform(VuoSceneObject object, VuoTransform transform)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->transform = transform;
}

/**
 * Changes the sceneobject's transform's translation.
 *
 * @version200New
 */
void VuoSceneObject_setTranslation(VuoSceneObject object, VuoPoint3d translation)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->transform.translation = translation;
}

/**
 * Changes the sceneobject's transform's scale.
 *
 * @version200New
 */
void VuoSceneObject_setScale(VuoSceneObject object, VuoPoint3d scale)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->transform.scale = scale;
}

/**
 * Changes the sceneobject's shader.
 *
 * @version200New
 */
void VuoSceneObject_setShader(VuoSceneObject object, VuoShader shader)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	VuoRetain(shader);
	VuoRelease(so->shader);
	so->shader = shader;
}

/**
 * Sets the `faceCulling` on `object` and its child objects.
 */
void VuoSceneObject_setFaceCulling(VuoSceneObject object, VuoMesh_FaceCulling faceCulling)
{
	if (!object)
		return;

	VuoSceneObject_apply(object, ^(VuoSceneObject currentObject, float modelviewMatrix[16]){
		VuoMesh m = VuoMesh_copyShallow(VuoSceneObject_getMesh(currentObject));
		VuoMesh_setFaceCulling(m, faceCulling);
		VuoSceneObject_setMesh(currentObject, m);
	});
}

/**
 * Sets the `blendMode` on `object` and its child objects.
 *
 * Only the following @ref VuoBlendMode%s are supported:
 *
 *    - @ref VuoBlendMode_Normal
 *    - @ref VuoBlendMode_Multiply
 *    - @ref VuoBlendMode_DarkerComponents
 *    - @ref VuoBlendMode_LighterComponents
 *    - @ref VuoBlendMode_LinearDodge
 *    - @ref VuoBlendMode_Subtract
 */
void VuoSceneObject_setBlendMode(VuoSceneObject object, VuoBlendMode blendMode)
{
	if (!object)
		return;

	VuoSceneObject_apply(object, ^(VuoSceneObject currentObject, float modelviewMatrix[16]){
		VuoSceneObject_internal *co = (VuoSceneObject_internal *)currentObject;
		co->blendMode = blendMode;
	});
}

/**
 * Changes whether the sceneobject should ignore rotations and scales
 * and be sized to match the shader's first image.
 *
 * @version200New
 */
void VuoSceneObject_setRealSize(VuoSceneObject object, bool isRealSize)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->isRealSize = isRealSize;
}

/**
 * Changes whether the sceneobject should use the texture's scaleFactor
 * and the backingScaleFactor to determine the rendered size.
 *
 * @version200New
 */
void VuoSceneObject_setPreservePhysicalSize(VuoSceneObject object, bool shouldPreservePhysicalSize)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->preservePhysicalSize = shouldPreservePhysicalSize;
}

/**
 * Changes the sceneobject's rendered text.
 *
 * @version200New
 */
void VuoSceneObject_setText(VuoSceneObject object, VuoText text)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	VuoRetain(text);
	VuoRelease(so->text.text);
	so->text.text = text;
}

/**
 * Changes the sceneobject's font for rendered text.
 *
 * @version200New
 */
void VuoSceneObject_setTextFont(VuoSceneObject object, VuoFont font)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	VuoFont_retain(font);
	VuoFont_release(so->text.font);
	so->text.font = font;
}

/**
 * Changes the sceneobject's camera field of view (for perspective cameras).
 *
 * @version200New
 */
void VuoSceneObject_setCameraFieldOfView(VuoSceneObject object, float fieldOfView)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->camera.fieldOfView = fieldOfView;
}

/**
 * Changes the sceneobject's depth buffer minimum distance.
 *
 * @version200New
 */
void VuoSceneObject_setCameraDistanceMin(VuoSceneObject object, float distanceMin)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->camera.distanceMin = distanceMin;
}

/**
 * Changes the sceneobject's depth buffer maximum distance.
 *
 * @version200New
 */
void VuoSceneObject_setCameraDistanceMax(VuoSceneObject object, float distanceMax)
{
	if (!object)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)object;
	so->camera.distanceMax = distanceMax;
}

/**
 * Creates a new scene object hierarchy that references the input object's meshes and shaders.
 *
 * You can change the transforms and _replace_ the meshes and shaders without affecting the original,
 * but you cannot _mutate_ the existing meshes and shaders.
 *
 * The sceneobject's id is preserved.
 *
 * @version200Changed{Meshes are now retained, not copied.}
 */
VuoSceneObject VuoSceneObject_copy(const VuoSceneObject object)
{
	if (!object)
		return nullptr;

	VuoSceneObject_internal *o = (VuoSceneObject_internal *)object;

	VuoSceneObject_internal *co = (VuoSceneObject_internal *)VuoSceneObject_makeEmpty();

	co->type = o->type;
	co->id = o->id;
	VuoSceneObject_setName((VuoSceneObject)co, o->name);
	co->transform = o->transform;
	VuoSceneObject_setMesh((VuoSceneObject)co, o->mesh);
	VuoSceneObject_setShader((VuoSceneObject)co, o->shader); // @todo
	co->isRealSize = o->isRealSize;
	co->preservePhysicalSize = o->preservePhysicalSize;
	co->blendMode = o->blendMode;

	if (o->type == VuoSceneObjectSubType_Group && o->childObjects)
	{
		co->childObjects = VuoListCreate_VuoSceneObject();
		VuoRetain(co->childObjects);
		VuoListForeach_VuoSceneObject(o->childObjects, ^(const VuoSceneObject object){
			VuoListAppendValue_VuoSceneObject(co->childObjects, VuoSceneObject_copy(object));
			return true;
		});
	}

	if (o->type == VuoSceneObjectSubType_PerspectiveCamera
	 || o->type == VuoSceneObjectSubType_StereoCamera
	 || o->type == VuoSceneObjectSubType_OrthographicCamera
	 || o->type == VuoSceneObjectSubType_FisheyeCamera)
	{
		co->camera.fieldOfView = o->camera.fieldOfView;
		co->camera.width = o->camera.width;
		co->camera.distanceMin = o->camera.distanceMin;
		co->camera.distanceMax = o->camera.distanceMax;
		co->camera.confocalDistance = o->camera.confocalDistance;
		co->camera.intraocularDistance = o->camera.intraocularDistance;
		co->camera.vignetteWidth = o->camera.vignetteWidth;
		co->camera.vignetteSharpness = o->camera.vignetteSharpness;
	}
	else if (o->type == VuoSceneObjectSubType_AmbientLight
		  || o->type == VuoSceneObjectSubType_PointLight
		  || o->type == VuoSceneObjectSubType_Spotlight)
	{
		co->light.color = o->light.color;
		co->light.brightness = o->light.brightness;
		co->light.range = o->light.range;
		co->light.cone = o->light.cone;
		co->light.sharpness = o->light.sharpness;
	}
	else if (o->type == VuoSceneObjectSubType_Text)
	{
		VuoSceneObject_setText((VuoSceneObject)co, o->text.text);
		VuoSceneObject_setTextFont((VuoSceneObject)co, o->text.font);
		co->text.scaleWithScene = o->text.scaleWithScene;
		co->text.wrapWidth = o->text.wrapWidth;
	}

	return (VuoSceneObject)co;
}

/**
  *	Get the axis aligned bounding box of this sceneobject and its children (and its children's children).
  */
VuoBox VuoSceneObject_bounds(const VuoSceneObject so)
{
	if (!so)
		return VuoBox_make((VuoPoint3d){0,0,0}, (VuoPoint3d){0,0,0});

	__block bool haveGlobalBounds = false;
	__block VuoBox globalBounds;

	VuoSceneObject_visit(so, ^(const VuoSceneObject currentObject, float modelviewMatrix[16]){
		VuoBox bounds;
		bool foundBounds = VuoSceneObject_meshBounds(currentObject, &bounds, modelviewMatrix);
		if (foundBounds)
		{
			globalBounds = haveGlobalBounds ? VuoBox_encapsulate(globalBounds, bounds) : bounds;
			haveGlobalBounds = true;
		}
		return true;
	});

	if (haveGlobalBounds)
		return globalBounds;
	else
		return VuoBox_make( (VuoPoint3d){0,0,0}, (VuoPoint3d){0,0,0} );
}

/**
 *	Bounding box of the vertices for this SceneObject (taking into account transform).
 */
bool VuoSceneObject_meshBounds(const VuoSceneObject sceneObject, VuoBox *bounds, float matrix[16])
{
	if (!sceneObject)
		return false;

	if (VuoSceneObject_getVertexCount(sceneObject) < 1)
		return false;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;
	if (so->isRealSize
	 || so->type == VuoSceneObjectSubType_Text)
	{
		// We don't know what the actual rendered size of the realSize layer will be,
		// but we can at least include its center point.
		*bounds = VuoBox_make(VuoPoint3d_make(matrix[12], matrix[13], matrix[14]), VuoPoint3d_make(0,0,0));
	}
	else
	{
		*bounds = VuoMesh_bounds(so->mesh, matrix);

		if (so->shader)
		{
			bounds->size.x *= so->shader->objectScale;
			bounds->size.y *= so->shader->objectScale;
			bounds->size.z *= so->shader->objectScale;
		}
	}

	return true;
}

/**
 *	Make the bounds center of all vertices == {0,0,0}
 */
void VuoSceneObject_center(VuoSceneObject sceneObject)
{
	if (!sceneObject)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;
	VuoBox bounds = VuoSceneObject_bounds(sceneObject);
	so->transform.translation = VuoPoint3d_subtract(so->transform.translation, bounds.center);
}

/**
 * Change the root sceneobject's transform such that the entire scenegraph renders within a 1x1x1 axis-aligned cube.
 * If the scenegraph has zero size (e.g., if it is empty, or if it consists entirely of Real Size Layers),
 * the transform is left unchanged.
 */
void VuoSceneObject_normalize(VuoSceneObject sceneObject)
{
	if (!sceneObject)
		return;
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;
	VuoBox bounds = VuoSceneObject_bounds(sceneObject);

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
 *     "preservePhysicalSize" : false,
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

	int id = 0;
	if (json_object_object_get_ex(js, "id", &o))
		id = json_object_get_int64(o);

	VuoSceneObjectSubType type = VuoSceneObjectSubType_Empty;
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

	bool preservePhysicalSize = false;
	if (json_object_object_get_ex(js, "preservePhysicalSize", &o))
		preservePhysicalSize = VuoBoolean_makeFromJson(o);

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
	if (json_object_object_get_ex(js, "textFont", &o))
		font = VuoFont_makeFromJson(o);

	bool scaleWithScene = false;
	if (json_object_object_get_ex(js, "textScaleWithScene", &o))
		scaleWithScene = VuoBoolean_makeFromJson(o);

	float wrapWidth = INFINITY;
	if (json_object_object_get_ex(js, "textWrapWidth", &o))
		wrapWidth = json_object_get_double(o);

	VuoSceneObject obj;
	switch (type)
	{
		case VuoSceneObjectSubType_Empty:
			obj = nullptr;
			break;
		case VuoSceneObjectSubType_Group:
			obj = VuoSceneObject_makeGroup(childObjects, transform);
			break;
		case VuoSceneObjectSubType_Mesh:
		{
			obj = VuoSceneObject_makeMesh(mesh, shader, transform);
			VuoSceneObject_internal *so = (VuoSceneObject_internal *)obj;
			so->isRealSize = isRealSize;
			so->preservePhysicalSize = preservePhysicalSize;
			so->blendMode = blendMode;
			VuoSceneObject_setName(obj, name);
			break;
		}
		case VuoSceneObjectSubType_PerspectiveCamera:
			obj = VuoSceneObject_makePerspectiveCamera(
						name,
						transform,
						cameraFieldOfView,
						cameraDistanceMin,
						cameraDistanceMax
						);
			break;
		case VuoSceneObjectSubType_StereoCamera:
			obj = VuoSceneObject_makeStereoCamera(
						name,
						transform,
						cameraFieldOfView,
						cameraDistanceMin,
						cameraDistanceMax,
						cameraConfocalDistance,
						cameraIntraocularDistance
						);
			break;
		case VuoSceneObjectSubType_OrthographicCamera:
			obj = VuoSceneObject_makeOrthographicCamera(
						name,
						transform,
						cameraWidth,
						cameraDistanceMin,
						cameraDistanceMax
						);
			break;
		case VuoSceneObjectSubType_FisheyeCamera:
			obj = VuoSceneObject_makeFisheyeCamera(
						name,
						transform,
						cameraFieldOfView,
						cameraVignetteWidth,
						cameraVignetteSharpness
						);
			break;
		case VuoSceneObjectSubType_AmbientLight:
			obj = VuoSceneObject_makeAmbientLight(lightColor, lightBrightness);
			break;
		case VuoSceneObjectSubType_PointLight:
			obj = VuoSceneObject_makePointLight(lightColor, lightBrightness, transform.translation, lightRange, lightSharpness);
			break;
		case VuoSceneObjectSubType_Spotlight:
			obj = VuoSceneObject_makeSpotlight(lightColor, lightBrightness, transform, lightCone, lightRange, lightSharpness);
			break;
		case VuoSceneObjectSubType_Text:
			obj = VuoSceneObject_makeText(text, font, scaleWithScene, wrapWidth);
			VuoSceneObject_setTransform(obj, transform);
			VuoSceneObject_setMesh(obj, mesh);
			break;
	}

	VuoSceneObject_setId(obj, id);

	return obj;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoSceneObject_getJson(const VuoSceneObject sceneObject)
{
	if (!sceneObject)
		return nullptr;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;

	json_object *js = json_object_new_object();

	json_object_object_add(js, "id", json_object_new_int64(so->id));
	json_object_object_add(js, "type", json_object_new_string(VuoSceneObject_cStringForType(so->type)));

	switch (so->type)
	{
		case VuoSceneObjectSubType_Empty:
			break;

		case VuoSceneObjectSubType_Mesh:
			if (so->mesh)
				json_object_object_add(js, "mesh", VuoMesh_getJson(so->mesh));

			if (so->shader)
				json_object_object_add(js, "shader", VuoShader_getJson(so->shader));

			json_object_object_add(js, "isRealSize", VuoBoolean_getJson(so->isRealSize));

			json_object_object_add(js, "preservePhysicalSize", VuoBoolean_getJson(so->preservePhysicalSize));

			if (so->blendMode != VuoBlendMode_Normal)
				json_object_object_add(js, "blendMode", VuoBlendMode_getJson(so->blendMode));
			break;

		case VuoSceneObjectSubType_Group:
			if (so->childObjects)
				json_object_object_add(js, "childObjects", VuoList_VuoSceneObject_getJson(so->childObjects));
			break;

		case VuoSceneObjectSubType_PerspectiveCamera:
		case VuoSceneObjectSubType_StereoCamera:
		case VuoSceneObjectSubType_OrthographicCamera:
		case VuoSceneObjectSubType_FisheyeCamera:
		{
			if (so->type != VuoSceneObjectSubType_FisheyeCamera)
			{
				json_object_object_add(js, "cameraDistanceMin", json_object_new_double(so->camera.distanceMin));
				json_object_object_add(js, "cameraDistanceMax", json_object_new_double(so->camera.distanceMax));
			}

			if (so->type == VuoSceneObjectSubType_PerspectiveCamera
			 || so->type == VuoSceneObjectSubType_StereoCamera
			 || so->type == VuoSceneObjectSubType_FisheyeCamera)
				json_object_object_add(js, "cameraFieldOfView", json_object_new_double(so->camera.fieldOfView));

			if (so->type == VuoSceneObjectSubType_StereoCamera)
			{
				json_object_object_add(js, "cameraConfocalDistance", json_object_new_double(so->camera.confocalDistance));
				json_object_object_add(js, "cameraIntraocularDistance", json_object_new_double(so->camera.intraocularDistance));
			}

			if (so->type == VuoSceneObjectSubType_OrthographicCamera)
				json_object_object_add(js, "cameraWidth", json_object_new_double(so->camera.width));

			if (so->type == VuoSceneObjectSubType_FisheyeCamera)
			{
				json_object_object_add(js, "cameraVignetteWidth", json_object_new_double(so->camera.vignetteWidth));
				json_object_object_add(js, "cameraVignetteSharpness", json_object_new_double(so->camera.vignetteSharpness));
			}

			break;
		}

		case VuoSceneObjectSubType_AmbientLight:
		case VuoSceneObjectSubType_PointLight:
		case VuoSceneObjectSubType_Spotlight:
		{
			json_object_object_add(js, "lightColor", VuoColor_getJson(so->light.color));
			json_object_object_add(js, "lightBrightness", json_object_new_double(so->light.brightness));

			if (so->type == VuoSceneObjectSubType_PointLight
			 || so->type == VuoSceneObjectSubType_Spotlight)
			{
				json_object_object_add(js, "lightRange", json_object_new_double(so->light.range));
				json_object_object_add(js, "lightSharpness", json_object_new_double(so->light.sharpness));
			}
			if (so->type == VuoSceneObjectSubType_Spotlight)
				json_object_object_add(js, "lightCone", json_object_new_double(so->light.cone));

			break;
		}

		case VuoSceneObjectSubType_Text:
		{
			if (so->text.text)
				json_object_object_add(js, "text", VuoText_getJson(so->text.text));

			json_object_object_add(js, "textFont", VuoFont_getJson(so->text.font));

			if (so->mesh)
				json_object_object_add(js, "mesh", VuoMesh_getJson(so->mesh));

			json_object_object_add(js, "textScaleWithScene", VuoBoolean_getJson(so->text.scaleWithScene));
			json_object_object_add(js, "textWrapWidth", VuoReal_getJson(so->text.wrapWidth));

			break;
		}
	}

	if (so->name)
		json_object_object_add(js, "name", VuoText_getJson(so->name));

	if (so->type != VuoSceneObjectSubType_AmbientLight)
		json_object_object_add(js, "transform", VuoTransform_getJson(so->transform));

	return js;
}

/**
 * Returns the total number of vertices in the scene object (but not its descendants).
 */
unsigned long VuoSceneObject_getVertexCount(const VuoSceneObject sceneObject)
{
	if (!sceneObject)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;
	if (!so->mesh)
		return 0;

	unsigned int vertexCount;
	VuoMesh_getCPUBuffers(so->mesh, &vertexCount, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
	return vertexCount;
}

/**
 * Returns the total number of element in the scene object (but not its descendants).
 */
unsigned long VuoSceneObject_getElementCount(const VuoSceneObject sceneObject)
{
	if (!sceneObject)
		return 0;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;
	if (!so->mesh)
		return 0;

	unsigned int elementCount;
	VuoMesh_getCPUBuffers(so->mesh, nullptr, nullptr, nullptr, nullptr, nullptr, &elementCount, nullptr);
	return elementCount;
}

/**
 * Traverses the specified scenegraph and returns statistics about it.
 *
 * The caller should initialize the output parameters to 0 before calling this function.
 */
void VuoSceneObject_getStatistics(const VuoSceneObject sceneObject, unsigned long *descendantCount, unsigned long *totalVertexCount, unsigned long *totalElementCount)
{
	if (!sceneObject)
		return;

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;
	unsigned long childObjectCount = 0;
	if (so->type == VuoSceneObjectSubType_Group && so->childObjects)
		childObjectCount = VuoListGetCount_VuoSceneObject(so->childObjects);
	*descendantCount += childObjectCount;
	*totalVertexCount += VuoSceneObject_getVertexCount(sceneObject);
	*totalElementCount += VuoSceneObject_getElementCount(sceneObject);

	if (so->type == VuoSceneObjectSubType_Group)
		for (unsigned long i = 1; i <= childObjectCount; ++i)
			VuoSceneObject_getStatistics(VuoListGetValue_VuoSceneObject(so->childObjects, i), descendantCount, totalVertexCount, totalElementCount);
}

/**
 * Returns a list of all unique shader names in the sceneobject and its descendants.
 */
static VuoList_VuoText VuoSceneObject_findShaderNames(VuoSceneObject sceneObject)
{
	if (!sceneObject)
		return nullptr;

	// Exploit json_object's set-containing-only-unique-items data structure.
	__block json_object *names = json_object_new_object();
	VuoSceneObject_visit(sceneObject, ^(const VuoSceneObject currentObject, float modelviewMatrix[16]){
		VuoSceneObject_internal *co = (VuoSceneObject_internal *)currentObject;
		if (co->shader)
			json_object_object_add(names, co->shader->name, NULL);
		return true;
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
char *VuoSceneObject_getSummary(const VuoSceneObject sceneObject)
{
	if (!VuoSceneObject_isPopulated(sceneObject))
		return strdup("No object");

	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;

	if (so->type == VuoSceneObjectSubType_Text)
	{
		char *fontSummary = VuoFont_getSummary(so->text.font);
		char *textSummary = VuoText_format("<div>\"%s\"</div><div>%sat (%g,%g)</div><div>ID %llu</div>", so->text.text, fontSummary, so->transform.translation.x, so->transform.translation.y, so->id);
		free(fontSummary);
		return textSummary;
	}

	if (so->type == VuoSceneObjectSubType_PerspectiveCamera
	 || so->type == VuoSceneObjectSubType_StereoCamera
	 || so->type == VuoSceneObjectSubType_OrthographicCamera
	 || so->type == VuoSceneObjectSubType_FisheyeCamera)
	{
		char *type = strdup(VuoSceneObject_cStringForType(so->type));
		type[0] = toupper(type[0]);

		float cameraViewValue = 0;
		const char *cameraViewString = "";
		if (so->type == VuoSceneObjectSubType_PerspectiveCamera)
		{
			cameraViewValue = so->camera.fieldOfView;
			cameraViewString = "° field of view";
		}
		else if (so->type == VuoSceneObjectSubType_StereoCamera)
		{
			cameraViewValue = so->camera.fieldOfView;
			cameraViewString = "° field of view (stereoscopic)";
		}
		else if (so->type == VuoSceneObjectSubType_OrthographicCamera)
		{
			cameraViewValue = so->camera.width;
			cameraViewString = " unit width";
		}
		else if (so->type == VuoSceneObjectSubType_FisheyeCamera)
		{
			cameraViewValue = so->camera.fieldOfView;
			cameraViewString = "° field of view (fisheye)";
		}

		char *translationString = VuoPoint3d_getSummary(so->transform.translation);

		const char *rotationLabel;
		char *rotationString;
		if (so->transform.type == VuoTransformTypeEuler)
		{
			rotationLabel = "Rotated";
			rotationString = VuoPoint3d_getSummary(VuoPoint3d_multiply(so->transform.rotationSource.euler, -180.f/M_PI));
		}
		else
		{
			rotationLabel = "Target";
			rotationString = VuoPoint3d_getSummary(so->transform.rotationSource.target);
		}

		char *valueAsString = VuoText_format("<div>%s named \"%s\"</div><div>At (%s)</div><div>%s (%s)</div><div>%g%s</div><div>Shows objects between depth %g and %g</div>",
											 type, so->name ? so->name : "",
											 translationString,
											 rotationLabel, rotationString,
											 cameraViewValue, cameraViewString,
											 so->camera.distanceMin, so->camera.distanceMax);
		free(rotationString);
		free(translationString);
		free(type);
		return valueAsString;
	}

	if (so->type == VuoSceneObjectSubType_AmbientLight
	 || so->type == VuoSceneObjectSubType_PointLight
	 || so->type == VuoSceneObjectSubType_Spotlight)
	{
		char *type = strdup(VuoSceneObject_cStringForType(so->type));
		type[0] = toupper(type[0]);

		char *colorString = VuoColor_getShortSummary(so->light.color);

		char *positionRangeString;
		if (so->type == VuoSceneObjectSubType_PointLight
		 || so->type == VuoSceneObjectSubType_Spotlight)
		{
			char *positionString = VuoPoint3d_getSummary(so->transform.translation);

			positionRangeString = VuoText_format("<div>Position (%s)</div><div>Range %g units (%g sharpness)</div>",
												 positionString, so->light.range, so->light.sharpness);

			free(positionString);
		}
		else
			positionRangeString = strdup("");

		char *directionConeString;
		if (so->type == VuoSceneObjectSubType_Spotlight)
		{
			VuoPoint3d direction = VuoTransform_getDirection(so->transform);
			char *directionString = VuoPoint3d_getSummary(direction);

			directionConeString = VuoText_format("<div>Direction (%s)</div><div>Cone %g°</div>",
												 directionString, so->light.cone * 180./M_PI);

			free(directionString);
		}
		else
			directionConeString = strdup("");

		char *valueAsString = VuoText_format("<div>%s</div><div>Color %s</div><div>Brightness %g</div>%s%s",
											 type, colorString, so->light.brightness, positionRangeString, directionConeString);

		free(directionConeString);
		free(positionRangeString);
		free(colorString);
		free(type);

		return valueAsString;
	}

	unsigned long vertexCount = VuoSceneObject_getVertexCount(sceneObject);
	unsigned long elementCount = VuoSceneObject_getElementCount(sceneObject);

	char *transform = VuoTransform_getSummary(so->transform);

	unsigned long childObjectCount = 0;
	if (so->type == VuoSceneObjectSubType_Group && so->childObjects)
		childObjectCount = VuoListGetCount_VuoSceneObject(so->childObjects);
	const char *childObjectPlural = childObjectCount == 1 ? "" : "s";

	char *descendants;
	if (childObjectCount)
	{
		unsigned long descendantCount = 0;
		unsigned long totalVertexCount = 0;
		unsigned long totalElementCount = 0;
		VuoSceneObject_getStatistics(sceneObject, &descendantCount, &totalVertexCount, &totalElementCount);
		const char *descendantPlural = descendantCount == 1 ? "" : "s";

		descendants = VuoText_format("<div>%ld descendant%s</div><div>Total, including descendants:</div><div>%ld vertices, %ld elements</div>",
									 descendantCount, descendantPlural, totalVertexCount, totalElementCount);
	}
	else
		descendants = strdup("");

	VuoList_VuoText shaderNames = VuoSceneObject_findShaderNames(sceneObject);
	VuoRetain(shaderNames);
	char *shaderNamesSummary;
	if (VuoListGetCount_VuoText(shaderNames))
	{
		VuoInteger shaderNameCount = VuoListGetCount_VuoText(shaderNames);
		const char *header = "<div>Shaders:<ul>";
		VuoInteger shaderNameLength = strlen(header);
		for (VuoInteger i = 1; i <= shaderNameCount; ++i)
			shaderNameLength += strlen("<li>") + strlen(VuoListGetValue_VuoText(shaderNames, i)) + strlen("</li>");
		shaderNameLength += strlen("</ul></div>");

		shaderNamesSummary = (char *)malloc(shaderNameLength + 1);
		char *t = shaderNamesSummary;
		t = strcpy(t, header) + strlen(header);
		for (VuoInteger i = 1; i <= shaderNameCount; ++i)
		{
			t = strcpy(t, "<li>") + strlen("<li>");
			t = strcpy(t, VuoListGetValue_VuoText(shaderNames, i)) + strlen(VuoListGetValue_VuoText(shaderNames, i));
			t = strcpy(t, "</li>") + strlen("</li>");
		}
		t = strcpy(t, "</ul></div>");
	}
	else
		shaderNamesSummary = strdup("");
	VuoRelease(shaderNames);

	char *name = NULL;
	if (so->name)
		name = VuoText_format("<div>Object named \"%s\"</div>", so->name);

	char *valueAsString = VuoText_format("%s<div>%ld vertices, %ld elements</div><div>%s</div><div>ID %lld</div><div>%ld child object%s</div>%s%s",
										 name ? name : "",
										 vertexCount, elementCount,
										 transform,
										 so->id,
										 childObjectCount, childObjectPlural,
										 descendants, shaderNamesSummary);

	free(descendants);
	free(transform);
	free(shaderNamesSummary);

	return valueAsString;
}

/**
 * Outputs information about the sceneobject (and its descendants).
 */
static void VuoSceneObject_dump_internal(const VuoSceneObject sceneObject, unsigned int level)
{
	VuoSceneObject_internal *so = (VuoSceneObject_internal *)sceneObject;

	for (unsigned int i=0; i<level; ++i)
		fprintf(stderr, "\t");

	if (!sceneObject)
	{
		fprintf(stderr, "no object\n");
		return;
	}

	fprintf(stderr, "%s (%s) ", VuoSceneObject_cStringForType(so->type), VuoTransform_getSummary(so->transform));
	if (so->type == VuoSceneObjectSubType_Mesh)
		fprintf(stderr, "%lu vertices, %lu elements, shader '%s' (%p)", VuoSceneObject_getVertexCount(sceneObject), VuoSceneObject_getElementCount(sceneObject), so->shader ? so->shader->name : "", so->shader);
	fprintf(stderr, "\n");

	if (so->type == VuoSceneObjectSubType_Group && so->childObjects)
	{
		unsigned int childObjectCount = VuoListGetCount_VuoSceneObject(so->childObjects);
		for (unsigned int i=1; i<=childObjectCount; ++i)
			VuoSceneObject_dump_internal(VuoListGetValue_VuoSceneObject(so->childObjects, i), level+1);
	}
}

/**
 * Outputs information about the sceneobject (and its descendants).
 */
void VuoSceneObject_dump(const VuoSceneObject so)
{
    VuoSceneObject_dump_internal(so,0);
}

/**
 * Combines all meshes (including child objects) together into a single mesh.
 *
 * Element assembly methods are expanded (e.g., triangle strips become individual triangles).
 *
 * If `so` contains multiple primitive types, a single mesh is created
 * with a submesh for each expanded primitive type.
 *
 * The last-visited submesh's `faceCullingMode` and `primitiveSize` are used in the output mesh.
 */
VuoSceneObject VuoSceneObject_flatten(const VuoSceneObject so)
{
	if (!so)
		return nullptr;

	// Count the vertices.

	__block unsigned long triangleVertexCount = 0;
	__block unsigned long trianglePrimitiveCount = 0;
	__block VuoMesh_FaceCulling triangleFaceCulling = VuoMesh_CullBackfaces;

	__block unsigned long lineVertexCount = 0;
	__block unsigned long linePrimitiveCount = 0;
	__block double        linePrimitiveSize = 0;

	__block unsigned long pointVertexCount = 0;
	__block unsigned long pointPrimitiveCount = 0;
	__block double        pointPrimitiveSize = 0;

	VuoSceneObject_visit(so, ^(const VuoSceneObject currentObject, float modelviewMatrix[16]){
		VuoSceneObject_internal *co = (VuoSceneObject_internal *)currentObject;
		if (!co->mesh)
			return true;

		VuoMesh_ElementAssemblyMethod elementAssemblyMethod = VuoMesh_getElementAssemblyMethod(co->mesh);

		unsigned int vertexCount;
		VuoMesh_getCPUBuffers(co->mesh, &vertexCount, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

			if (elementAssemblyMethod == VuoMesh_IndividualTriangles
			 || elementAssemblyMethod == VuoMesh_TriangleStrip
			 || elementAssemblyMethod == VuoMesh_TriangleFan)
			{
				triangleVertexCount += vertexCount;
				trianglePrimitiveCount += VuoMesh_getSplitPrimitiveCount(co->mesh);
				triangleFaceCulling = VuoMesh_getFaceCulling(co->mesh);
			}
			else if (elementAssemblyMethod == VuoMesh_IndividualLines
				  || elementAssemblyMethod == VuoMesh_LineStrip)
			{
				lineVertexCount += vertexCount;
				linePrimitiveCount += VuoMesh_getSplitPrimitiveCount(co->mesh);
				linePrimitiveSize = VuoMesh_getPrimitiveSize(co->mesh);
			}
			else if (elementAssemblyMethod == VuoMesh_Points)
			{
				pointVertexCount += vertexCount;
				pointPrimitiveCount += VuoMesh_getSplitPrimitiveCount(co->mesh);
				pointPrimitiveSize = VuoMesh_getPrimitiveSize(co->mesh);
			}

		return true;
	});
//	VLog("triangles: %ldv %ldp    lines: %ldv %ldp    points: %ldv %ldp", triangleVertexCount, trianglePrimitiveCount, lineVertexCount, linePrimitiveCount, pointVertexCount, pointPrimitiveCount);

	if (!trianglePrimitiveCount && !linePrimitiveCount && !pointPrimitiveCount)
		return nullptr;

	// Allocate the buffers.
	unsigned int *triangleElements = nullptr;
	float *trianglePositions = nullptr, *triangleNormals = nullptr, *triangleTextureCoordinates = nullptr;
	unsigned int *lineElements = nullptr;
	float *linePositions = nullptr, *lineNormals = nullptr, *lineTextureCoordinates = nullptr;
	unsigned int *pointElements = nullptr;
	float *pointPositions = nullptr, *pointNormals = nullptr, *pointTextureCoordinates = nullptr;
	if (trianglePrimitiveCount)
		VuoMesh_allocateCPUBuffers(triangleVertexCount, &trianglePositions, &triangleNormals, &triangleTextureCoordinates, nullptr, trianglePrimitiveCount * 3, &triangleElements);
	if (linePrimitiveCount)
		VuoMesh_allocateCPUBuffers(lineVertexCount, &linePositions, &lineNormals, &lineTextureCoordinates, nullptr, linePrimitiveCount * 2, &lineElements);
	if (pointPrimitiveCount)
		VuoMesh_allocateCPUBuffers(pointVertexCount, &pointPositions, &pointNormals, &pointTextureCoordinates, nullptr, pointPrimitiveCount, &pointElements);

	// Copy the vertex attributes.
	__block unsigned long triangleVertexIndex = 0;
	__block unsigned long triangleElementIndex  = 0;
	__block unsigned long lineVertexIndex = 0;
	__block unsigned long lineElementIndex  = 0;
	__block unsigned long pointVertexIndex = 0;
	__block unsigned long pointElementIndex  = 0;
	__block bool anyTextureCoordinates = false;
	VuoSceneObject_visit(so, ^(const VuoSceneObject currentObject, float modelviewMatrix[16]){
		VuoSceneObject_internal *co = (VuoSceneObject_internal *)currentObject;
		if (!co->mesh)
			return true;

		VuoMesh_ElementAssemblyMethod elementAssemblyMethod = VuoMesh_getElementAssemblyMethod(co->mesh);

		unsigned int vertexCount, elementCount, *elements;
		float *positions, *normals, *textureCoordinates;
		VuoMesh_getCPUBuffers(co->mesh, &vertexCount, &positions, &normals, &textureCoordinates, nullptr, &elementCount, &elements);

			if (textureCoordinates)
				anyTextureCoordinates = true;

			if (elementAssemblyMethod == VuoMesh_IndividualTriangles
			 || elementAssemblyMethod == VuoMesh_TriangleStrip
			 || elementAssemblyMethod == VuoMesh_TriangleFan)
			{
				unsigned long indexOffset = triangleVertexIndex;
				for (unsigned int n = 0; n < vertexCount; ++n)
				{
					VuoPoint3d p = VuoPoint3d_makeFromArray(&positions[n * 3]);
					VuoPoint3d pt = VuoTransform_transformPoint((float*)modelviewMatrix, p);
					VuoPoint3d_setArray(&trianglePositions[triangleVertexIndex * 3], pt);

					if (normals)
					{
						VuoPoint3d r = VuoPoint3d_makeFromArray(&normals[n * 3]);
						VuoPoint3d rt = VuoTransform_transformVector((float*)modelviewMatrix, r);
						VuoPoint3d_setArray(&triangleNormals[triangleVertexIndex * 3], rt);
					}

					if (textureCoordinates)
						VuoPoint2d_setArray(&triangleTextureCoordinates[triangleVertexIndex * 2], VuoPoint2d_makeFromArray(&textureCoordinates[n * 2]));

					++triangleVertexIndex;
				}

				if (elementAssemblyMethod == VuoMesh_IndividualTriangles)
				{
					if (elementCount)
						for (unsigned int n = 0; n < elementCount; ++n)
							triangleElements[triangleElementIndex++] = indexOffset + elements[n];
					else
						for (unsigned int n = 0; n < vertexCount; ++n)
							triangleElements[triangleElementIndex++] = indexOffset + n;
				}
				else if (elementAssemblyMethod == VuoMesh_TriangleStrip)
				{
					// Expand the triangle strip to individual triangles.
					if (elementCount)
						for (unsigned int n = 2; n < elementCount; ++n)
							if (n%2 == 0)
							{
								triangleElements[triangleElementIndex++] = indexOffset + elements[n-2];
								triangleElements[triangleElementIndex++] = indexOffset + elements[n-1];
								triangleElements[triangleElementIndex++] = indexOffset + elements[n  ];
							}
							else
							{
								triangleElements[triangleElementIndex++] = indexOffset + elements[n-1];
								triangleElements[triangleElementIndex++] = indexOffset + elements[n-2];
								triangleElements[triangleElementIndex++] = indexOffset + elements[n  ];
							}
					else
						for (unsigned int n = 0; n < vertexCount; ++n)
							if (n%2 == 0)
							{
								triangleElements[triangleElementIndex++] = indexOffset + n-2;
								triangleElements[triangleElementIndex++] = indexOffset + n-1;
								triangleElements[triangleElementIndex++] = indexOffset + n  ;
							}
							else
							{
								triangleElements[triangleElementIndex++] = indexOffset + n-1;
								triangleElements[triangleElementIndex++] = indexOffset + n-2;
								triangleElements[triangleElementIndex++] = indexOffset + n  ;
							}
				}
				else if (elementAssemblyMethod == VuoMesh_TriangleFan)
				{
					// Expand the triangle fan to individual triangles.
					if (elementCount)
						for (unsigned int n = 2; n < elementCount; ++n)
						{
							triangleElements[triangleElementIndex++] = indexOffset + elements[0  ];
							triangleElements[triangleElementIndex++] = indexOffset + elements[n-1];
							triangleElements[triangleElementIndex++] = indexOffset + elements[n  ];
						}
					else
						for (unsigned int n = 2; n < vertexCount; ++n)
						{
							triangleElements[triangleElementIndex++] = indexOffset + 0;
							triangleElements[triangleElementIndex++] = indexOffset + n-1;
							triangleElements[triangleElementIndex++] = indexOffset + n;
						}
				}
			}
			else if (elementAssemblyMethod == VuoMesh_IndividualLines
				  || elementAssemblyMethod == VuoMesh_LineStrip)
			{
				unsigned long indexOffset = lineVertexIndex;
				for (unsigned int n = 0; n < vertexCount; ++n)
				{
					VuoPoint3d p = VuoPoint3d_makeFromArray(&positions[n * 3]);
					VuoPoint3d pt = VuoTransform_transformPoint((float*)modelviewMatrix, p);
					VuoPoint3d_setArray(&linePositions[lineVertexIndex * 3], pt);

					if (normals)
					{
						VuoPoint3d r = VuoPoint3d_makeFromArray(&normals[n * 3]);
						VuoPoint3d rt = VuoTransform_transformVector((float*)modelviewMatrix, r);
						VuoPoint3d_setArray(&lineNormals[lineVertexIndex * 3], rt);
					}

					if (textureCoordinates)
						VuoPoint2d_setArray(&lineTextureCoordinates[lineVertexIndex], VuoPoint2d_makeFromArray(&textureCoordinates[n * 2]));

					++lineVertexIndex;
				}

				if (elementAssemblyMethod == VuoMesh_IndividualLines)
				{
					if (elementCount)
						for (unsigned int n = 0; n < elementCount; ++n)
							lineElements[lineElementIndex++] = indexOffset + elements[n];
					else
						for (unsigned int n = 0; n < vertexCount; ++n)
							lineElements[lineElementIndex++] = indexOffset + n;
				}
				else if (elementAssemblyMethod == VuoMesh_LineStrip)
				{
					// Expand the line strip to individual lines.
					if (elementCount)
						for (unsigned int n = 1; n < elementCount; ++n)
						{
							lineElements[lineElementIndex++] = indexOffset + elements[n-1];
							lineElements[lineElementIndex++] = indexOffset + elements[n  ];
						}
					else
						for (unsigned int n = 1; n < vertexCount; ++n)
						{
							lineElements[lineElementIndex++] = indexOffset + n-1;
							lineElements[lineElementIndex++] = indexOffset + n;
						}
				}
			}
			else if (elementAssemblyMethod == VuoMesh_Points)
			{
				unsigned long indexOffset = pointVertexIndex;
				for (unsigned int n = 0; n < vertexCount; ++n)
				{
					VuoPoint3d p = VuoPoint3d_makeFromArray(&positions[n * 3]);
					VuoPoint3d pt = VuoTransform_transformPoint((float*)modelviewMatrix, p);
					VuoPoint3d_setArray(&pointPositions[pointVertexIndex * 3], pt);

					if (normals)
					{
						VuoPoint3d r = VuoPoint3d_makeFromArray(&normals[n * 3]);
						VuoPoint3d rt = VuoTransform_transformVector((float*)modelviewMatrix, r);
						VuoPoint3d_setArray(&pointNormals[pointVertexIndex * 3], rt);
					}

					if (textureCoordinates)
						VuoPoint2d_setArray(&pointTextureCoordinates[pointVertexIndex * 2], VuoPoint2d_makeFromArray(&textureCoordinates[n * 2]));

					++pointVertexIndex;
				}

				if (elementCount)
					/// @todo It doesn't really make sense for a point mesh to have an element buffer, does it?
					for (unsigned int n = 0; n < elementCount; ++n)
						pointElements[pointElementIndex++] = indexOffset + elements[n];
				else
				{
					for (unsigned int n = 0; n < vertexCount; ++n)
						pointElements[pointElementIndex++] = indexOffset + n;
				}
			}

		return true;
	});


	VuoMesh triangleMesh = nullptr;
	VuoMesh lineMesh = nullptr;
	VuoMesh pointMesh = nullptr;
	if (trianglePrimitiveCount)
	{
		if (!anyTextureCoordinates)
		{
			free(triangleTextureCoordinates);
			triangleTextureCoordinates = nullptr;
		}
		triangleMesh = VuoMesh_makeFromCPUBuffers(triangleVertexCount, trianglePositions, triangleNormals, triangleTextureCoordinates, nullptr, trianglePrimitiveCount * 3, triangleElements, VuoMesh_IndividualTriangles);
		VuoMesh_setFaceCulling(triangleMesh, triangleFaceCulling);
	}
	if (linePrimitiveCount)
	{
		if (!anyTextureCoordinates)
		{
			free(lineTextureCoordinates);
			lineTextureCoordinates = nullptr;
		}
		lineMesh = VuoMesh_makeFromCPUBuffers(lineVertexCount, linePositions, lineNormals, lineTextureCoordinates, nullptr, linePrimitiveCount * 2, lineElements, VuoMesh_IndividualLines);
		VuoMesh_setPrimitiveSize(lineMesh, linePrimitiveSize);
	}
	if (pointPrimitiveCount)
	{
		if (!anyTextureCoordinates)
		{
			free(pointTextureCoordinates);
			pointTextureCoordinates = nullptr;
		}
		pointMesh = VuoMesh_makeFromCPUBuffers(pointVertexCount, pointPositions, pointNormals, pointTextureCoordinates, nullptr, pointPrimitiveCount, pointElements, VuoMesh_Points);
		VuoMesh_setPrimitiveSize(pointMesh, pointPrimitiveSize);
	}

	if (triangleMesh && !lineMesh && !pointMesh)
		return VuoSceneObject_makeMesh(triangleMesh, NULL, VuoTransform_makeIdentity());
	else if (!triangleMesh && lineMesh && !pointMesh)
		return VuoSceneObject_makeMesh(lineMesh, NULL, VuoTransform_makeIdentity());
	else if (!triangleMesh && !lineMesh && pointMesh)
		return VuoSceneObject_makeMesh(pointMesh, NULL, VuoTransform_makeIdentity());
	else
	{
		VuoList_VuoSceneObject childObjects = VuoListCreate_VuoSceneObject();
		if (triangleMesh)
			VuoListAppendValue_VuoSceneObject(childObjects, VuoSceneObject_makeMesh(triangleMesh, NULL, VuoTransform_makeIdentity()));
		if (lineMesh)
			VuoListAppendValue_VuoSceneObject(childObjects, VuoSceneObject_makeMesh(lineMesh, NULL, VuoTransform_makeIdentity()));
		if (pointMesh)
			VuoListAppendValue_VuoSceneObject(childObjects, VuoSceneObject_makeMesh(pointMesh, NULL, VuoTransform_makeIdentity()));
		return VuoSceneObject_makeGroup(childObjects, VuoTransform_makeIdentity());
	}

	return NULL;
}

#define CSGJS_HEADER_ONLY
#include "csgjs.cc"

/**
 * Converts a @ref VuoSceneObject to a `csgjs_model`.
 */
static csgjs_model VuoSceneObject_getCsgjsModel(const VuoSceneObject so)
{
	if (!so)
		return csgjs_model();

	VuoSceneObject flat = VuoSceneObject_flatten(so);
	VuoSceneObject_internal *f = (VuoSceneObject_internal *)flat;
	if (!f->mesh)
		return csgjs_model();

	VuoLocal(flat);

	if (VuoMesh_getElementAssemblyMethod(f->mesh) != VuoMesh_IndividualTriangles)
		return csgjs_model();

	unsigned int vertexCount, elementCount, *elements;
	float *positions, *normals, *textureCoordinates;
	VuoMesh_getCPUBuffers(f->mesh, &vertexCount, &positions, &normals, &textureCoordinates, nullptr, &elementCount, &elements);

	csgjs_model cm;
	for (unsigned int n = 0; n < vertexCount; ++n)
	{
		csgjs_vertex v;
		v.pos = csgjs_vector(positions[n * 3], positions[n * 3 + 1], positions[n * 3 + 2]);
		if (normals)
			v.normal = csgjs_vector(normals[n * 3], normals[n * 3 + 1], normals[n * 3 + 2]);
		if (textureCoordinates)
			v.uv = csgjs_vector(textureCoordinates[n * 3], textureCoordinates[n * 3 + 1], 0);
		cm.vertices.push_back(v);
	}
	for (unsigned int n = 0; n < elementCount; ++n)
		cm.indices.push_back(elements[n]);

	return cm;
}

/**
 * Converts a `csgjs_model` to a @ref VuoSceneObject.
 */
static VuoSceneObject VuoSceneObject_makeFromCsgjsModel(const csgjs_model &cm)
{
	unsigned int vertexCount = cm.vertices.size();
	unsigned int elementCount = cm.indices.size();
	unsigned int *elements;
	float *positions, *normals, *textureCoordinates;
	VuoMesh_allocateCPUBuffers(vertexCount, &positions, &normals, &textureCoordinates, nullptr, elementCount, &elements);

	const csgjs_vertex *vertex = &cm.vertices[0];
	for (unsigned int n = 0; n < vertexCount; ++n)
	{
		positions[n * 3    ] = vertex[n].pos.x;
		positions[n * 3 + 1] = vertex[n].pos.y;
		positions[n * 3 + 2] = vertex[n].pos.z;
		normals[n * 3    ] = vertex[n].normal.x;
		normals[n * 3 + 1] = vertex[n].normal.y;
		normals[n * 3 + 2] = vertex[n].normal.z;
		textureCoordinates[n * 2    ] = vertex[n].uv.x;
		textureCoordinates[n * 2 + 1] = vertex[n].uv.y;
	}

	const int *index = &cm.indices[0];
	for (unsigned int n = 0; n < elementCount; ++n)
		elements[n] = index[n];

	VuoMesh mesh = VuoMesh_makeFromCPUBuffers(vertexCount, positions, normals, textureCoordinates, nullptr, elementCount, elements, VuoMesh_IndividualTriangles);

	return VuoSceneObject_makeMesh(mesh, NULL, VuoTransform_makeIdentity());
}

/**
 * - quality 0 = epsilon 1
 * - quality 1 = epsilon 0.00001
 */
static float convertQualityToEpsilon(float quality)
{
	return pow(10, -VuoReal_clamp(quality, 0, 1) * 5.);
}

/**
 * Returns the union of `objects`.
 */
VuoSceneObject VuoSceneObject_union(VuoList_VuoSceneObject objects, float quality)
{
	float epsilon = convertQualityToEpsilon(quality);

	unsigned long objectCount = VuoListGetCount_VuoSceneObject(objects);
	if (objectCount == 0)
		return nullptr;
	if (objectCount == 1)
		return VuoListGetValue_VuoSceneObject(objects, 1);

	dispatch_queue_t queue = dispatch_queue_create("org.vuo.sceneobject.union", DISPATCH_QUEUE_CONCURRENT);

	csgjs_model *models = new csgjs_model[objectCount];
	for (unsigned long i = 0; i < objectCount; ++i)
		dispatch_async(queue, ^{
			models[i] = VuoSceneObject_getCsgjsModel(VuoListGetValue_VuoSceneObject(objects, i+1));
		});

	dispatch_barrier_sync(queue, ^{});
	dispatch_release(queue);

	csgjs_model cu = models[0];
	for (unsigned long i = 1; i < objectCount; ++i)
		cu = csgjs_union(cu, models[i], epsilon);
	delete[] models;
	return VuoSceneObject_makeFromCsgjsModel(cu);
}

/**
 * Returns the boolean difference of `a` minus `b`.
 */
VuoSceneObject VuoSceneObject_subtract(const VuoSceneObject a, const VuoSceneObject b, float quality)
{
	float epsilon = convertQualityToEpsilon(quality);

	dispatch_semaphore_t finished = dispatch_semaphore_create(0);

	__block csgjs_model ca;
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		ca = VuoSceneObject_getCsgjsModel(a);
		dispatch_semaphore_signal(finished);
	});

	csgjs_model cb = VuoSceneObject_getCsgjsModel(b);

	dispatch_semaphore_wait(finished, DISPATCH_TIME_FOREVER);
	dispatch_release(finished);

	csgjs_model d = csgjs_difference(ca, cb, epsilon);
	return VuoSceneObject_makeFromCsgjsModel(d);
}

/**
 * Returns the intersection of `objects`.
 */
VuoSceneObject VuoSceneObject_intersect(VuoList_VuoSceneObject objects, float quality)
{
	float epsilon = convertQualityToEpsilon(quality);

	unsigned long objectCount = VuoListGetCount_VuoSceneObject(objects);
	if (objectCount == 0)
		return nullptr;
	if (objectCount == 1)
		return VuoListGetValue_VuoSceneObject(objects, 1);

	dispatch_queue_t queue = dispatch_queue_create("org.vuo.sceneobject.intersect", DISPATCH_QUEUE_CONCURRENT);

	csgjs_model *models = new csgjs_model[objectCount];
	for (unsigned long i = 0; i < objectCount; ++i)
		dispatch_async(queue, ^{
			models[i] = VuoSceneObject_getCsgjsModel(VuoListGetValue_VuoSceneObject(objects, i+1));
		});

	dispatch_barrier_sync(queue, ^{});
	dispatch_release(queue);

	csgjs_model ci = models[0];
	for (unsigned long i = 1; i < objectCount; ++i)
		ci = csgjs_intersection(ci, models[i], epsilon);
	delete[] models;
	return VuoSceneObject_makeFromCsgjsModel(ci);
}
