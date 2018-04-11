/**
 * @file
 * VuoSceneText implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
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
	unsigned int vertexCount = 4;

	VuoPoint4d *positions = (VuoPoint4d *)malloc(sizeof(VuoPoint4d) * vertexCount);
	positions[0] = VuoPoint4d_make(-.5,-.5,0,1);
	positions[1] = VuoPoint4d_make( .5,-.5,0,1);
	positions[2] = VuoPoint4d_make(-.5, .5,0,1);
	positions[3] = VuoPoint4d_make( .5, .5,0,1);

	VuoPoint4d *textureCoordinates = (VuoPoint4d *)malloc(sizeof(VuoPoint4d) * vertexCount);
	textureCoordinates[0] = VuoPoint4d_make(0,0,0,1);
	textureCoordinates[1] = VuoPoint4d_make(1,0,0,1);
	textureCoordinates[2] = VuoPoint4d_make(0,1,0,1);
	textureCoordinates[3] = VuoPoint4d_make(1,1,0,1);

	unsigned int elementCount = 6;
	unsigned int *elements = (unsigned int *)malloc(sizeof(unsigned int) * elementCount);
	// Order the elements so that the diagonal edge of each triangle
	// is last, so that vuo.shader.make.wireframe can optionally omit them.
	elements[0] = 2;
	elements[1] = 0;
	elements[2] = 1;
	elements[3] = 1;
	elements[4] = 3;
	elements[5] = 2;

	float horizontal = anchor.horizontalAlignment == VuoHorizontalAlignment_Left ? .5f : (anchor.horizontalAlignment == VuoHorizontalAlignment_Right ? -.5f : 0.f);
	float vertical = anchor.verticalAlignment == VuoVerticalAlignment_Top ? -.5f : (anchor.verticalAlignment == VuoVerticalAlignment_Bottom ? .5f : 0.f);

	for(int i = 0; i < vertexCount; i++)
	{
		positions[i].x += horizontal;
		positions[i].y += vertical;
	}

	VuoSubmesh sm = VuoSubmesh_makeFromBuffers(vertexCount,
											   positions, NULL, NULL, NULL, textureCoordinates,
											   elementCount, elements, VuoMesh_IndividualTriangles);
	return VuoMesh_makeFromSingleSubmesh(sm);
}

VuoSceneObject VuoSceneText_make(const VuoText text, const VuoFont font, const VuoAnchor anchor)
{
	VuoSceneObject so = VuoSceneObject_makeText(text, font);
	so.mesh = makeAnchoredQuad(anchor);
	return so;
}
