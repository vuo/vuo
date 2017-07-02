/**
 * @file
 * VuoSceneText implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoSceneText.h"
#include "VuoMesh.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
#include "VuoVerticalAlignment.h"
#include "VuoHorizontalAlignment.h"
#include "node.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoSceneText",
					"dependencies" : [
						"VuoSceneObject",
						"VuoFont",
						"VuoMesh"
					]
				 });
#endif

/**
 * Create a new 4 point plane with positions set such that scaling will extend the mesh in the direction of anchor.
 */
/**
 * Creates a quad mesh anchored to the specified position.
 */
static VuoMesh makeAnchoredQuad(VuoAnchor anchor)
{
	VuoSubmesh sm;

	sm.vertexCount = 4;

	sm.positions = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*sm.vertexCount);
	{
		sm.positions[0] = VuoPoint4d_make(-.5,-.5,0,1);
		sm.positions[1] = VuoPoint4d_make( .5,-.5,0,1);
		sm.positions[2] = VuoPoint4d_make(-.5, .5,0,1);
		sm.positions[3] = VuoPoint4d_make( .5, .5,0,1);
	}

	sm.normals = NULL;
	sm.tangents = NULL;
	sm.bitangents = NULL;

	sm.textureCoordinates = (VuoPoint4d *)malloc(sizeof(VuoPoint4d)*sm.vertexCount);
	{
		sm.textureCoordinates[0] = VuoPoint4d_make(0,0,0,1);
		sm.textureCoordinates[1] = VuoPoint4d_make(1,0,0,1);
		sm.textureCoordinates[2] = VuoPoint4d_make(0,1,0,1);
		sm.textureCoordinates[3] = VuoPoint4d_make(1,1,0,1);
	}

	sm.elementCount = 6;
	sm.elements = (unsigned int *)malloc(sizeof(unsigned int)*sm.elementCount);
	sm.elementAssemblyMethod = VuoMesh_IndividualTriangles;
	sm.faceCullingMode = GL_BACK;

	// Order the elements so that the diagonal edge of each triangle
	// is last, so that vuo.shader.make.wireframe can optionally omit them.
	sm.elements[0] = 2;
	sm.elements[1] = 0;
	sm.elements[2] = 1;
	sm.elements[3] = 1;
	sm.elements[4] = 3;
	sm.elements[5] = 2;

	float horizontal = anchor.horizontalAlignment == VuoHorizontalAlignment_Left ? .5f : (anchor.horizontalAlignment == VuoHorizontalAlignment_Right ? -.5f : 0.f);
	float vertical = anchor.verticalAlignment == VuoVerticalAlignment_Top ? -.5f : (anchor.verticalAlignment == VuoVerticalAlignment_Bottom ? .5f : 0.f);

	for(int i = 0; i < sm.vertexCount; i++)
	{
		sm.positions[i].x += horizontal;
		sm.positions[i].y += vertical;
	}

	return VuoMesh_makeFromSingleSubmesh(sm);
}

VuoSceneObject VuoSceneText_make(const VuoText text, const VuoFont font, const VuoAnchor anchor)
{
	VuoSceneObject so = VuoSceneObject_makeText(text, font);
	so.mesh = makeAnchoredQuad(anchor);
	return so;
}
