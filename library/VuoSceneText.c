/**
 * @file
 * VuoSceneText implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSceneText.h"
#include "VuoImageText.h"
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoSceneText",
					"dependencies" : [
						"VuoImageText",
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
	unsigned int elementCount = 6;

	float *positions, *textureCoordinates;
	unsigned int *elements;
	VuoMesh_allocateCPUBuffers(vertexCount, &positions, NULL, &textureCoordinates, NULL, elementCount, &elements);

	positions[0 * 3    ] = -.5;
	positions[0 * 3 + 1] = -.5;
	positions[0 * 3 + 2] = 0;
	positions[1 * 3    ] =  .5;
	positions[1 * 3 + 1] = -.5;
	positions[1 * 3 + 2] = 0;
	positions[2 * 3    ] = -.5;
	positions[2 * 3 + 1] =  .5;
	positions[2 * 3 + 2] = 0;
	positions[3 * 3    ] =  .5;
	positions[3 * 3 + 1] =  .5;
	positions[3 * 3 + 2] = 0;

	textureCoordinates[0 * 2    ] = 0;
	textureCoordinates[0 * 2 + 1] = 0;
	textureCoordinates[1 * 2    ] = 1;
	textureCoordinates[1 * 2 + 1] = 0;
	textureCoordinates[2 * 2    ] = 0;
	textureCoordinates[2 * 2 + 1] = 1;
	textureCoordinates[3 * 2    ] = 1;
	textureCoordinates[3 * 2 + 1] = 1;

	// Order the elements so that the diagonal edge of each triangle
	// is last, so that vuo.shader.make.wireframe can optionally omit them.
	elements[0] = 2;
	elements[1] = 0;
	elements[2] = 1;
	elements[3] = 1;
	elements[4] = 3;
	elements[5] = 2;

	float horizontal = VuoAnchor_getHorizontal(anchor) == VuoHorizontalAlignment_Left ? .5f : (VuoAnchor_getHorizontal(anchor) == VuoHorizontalAlignment_Right ? -.5f : 0.f);
	float vertical = VuoAnchor_getVertical(anchor) == VuoVerticalAlignment_Top ? -.5f : (VuoAnchor_getVertical(anchor) == VuoVerticalAlignment_Bottom ? .5f : 0.f);

	for(int i = 0; i < vertexCount; i++)
	{
		positions[i * 3    ] += horizontal;
		positions[i * 3 + 1] += vertical;
	}

	return VuoMesh_makeFromCPUBuffers(vertexCount,
		positions, NULL, textureCoordinates, NULL,
		elementCount, elements, VuoMesh_IndividualTriangles);
}

/**
 * Creates a new VuoSceneObject set up with the correct font, shader, and mesh anchoring for text.
 *
 * @param text The text to render.
 * @param font The font to render the text with.
 * @param scaleWithScene If true, the text scales when its part of the scenegraph, or the viewport, is scaled.  If false, the text is rendered at a constant size.
 * @param wrapWidth The width (in Vuo Coordinates) at which to wrap lines, or `INFINITY` to disable wrapping.
 * @param anchor The position of the transform anchor.
 * @version200Changed{Added `scaleWithScene`, `wrapWidth` arguments.}
 */
VuoSceneObject VuoSceneText_make(const VuoText text, const VuoFont font, const VuoBoolean scaleWithScene, const VuoReal wrapWidth, const VuoAnchor anchor)
{
	VuoSceneObject so = VuoSceneObject_makeText(text, font, scaleWithScene, wrapWidth);
	VuoSceneObject_setMesh(so, makeAnchoredQuad(anchor));
	return so;
}

/**
 * Returns the anchor used when @ref VuoSceneText_make was called.
 * @version200New
 */
VuoAnchor VuoSceneText_getAnchor(VuoSceneObject so)
{
	VuoMesh mesh = VuoSceneObject_getMesh(so);
	if (!mesh)
		return VuoAnchor_makeCentered();

	VuoPoint2d mesh0;
	{
		unsigned int vertexCount;
		float *positions;
		VuoMesh_getCPUBuffers(mesh, &vertexCount, &positions, NULL, NULL, NULL, NULL, NULL);
		mesh0 = (VuoPoint2d){ positions[0], positions[1] };
	}

	VuoHorizontalAlignment h;
	VuoVerticalAlignment v;

	// (We know from VuoSceneText_make() that the mesh's first coordinate
	// is 0.0, -0.5, or -1.0, depending on whether the anchor is left/top, center, or right/bottom, respectively.)
	if (VuoReal_areEqual(mesh0.x, 0))
		h = VuoHorizontalAlignment_Left;
	else if (VuoReal_areEqual(mesh0.x, -.5))
		h = VuoHorizontalAlignment_Center;
	else //if (VuoReal_areEqual(mesh0.x, -1))
		h = VuoHorizontalAlignment_Right;

	if (VuoReal_areEqual(mesh0.y, -1))
		v = VuoVerticalAlignment_Top;
	else if (VuoReal_areEqual(mesh0.y, -.5))
		v = VuoVerticalAlignment_Center;
	else //if (VuoReal_areEqual(mesh0.y, 0))
		v = VuoVerticalAlignment_Bottom;

	return VuoAnchor_make(h, v);
}

/**
 * Returns the amount to translate the object to compensate for the anchor.
 * @version200New
 */
VuoPoint2d VuoSceneText_getAnchorOffset(VuoSceneObject so, float verticalScale, float rotationZ, float wrapWidth, int viewportWidth, int backingScaleFactor)
{
	VuoAnchor a = VuoSceneText_getAnchor(so);
	if (a == VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center))
		// Already in center.
		return (VuoPoint2d){0, 0};

	VuoImageTextData td = VuoImage_getTextImageData(VuoSceneObject_getText(so), VuoSceneObject_getTextFont(so), backingScaleFactor, verticalScale, rotationZ, false);
	VuoLocal(td);
	VuoPoint2d *corners = td->transformedCorners;

	// Convert the corner pixel coodrinates to scene coordinates.
	for (int i = 0; i < 4; ++i)
	{
		corners[i].x *= -2. / viewportWidth;
		corners[i].y *= -2. / viewportWidth;
	}

	// Move the origin to the anchor point.
	double width  = fmax(fabs(corners[2].x - corners[0].x), fabs(corners[3].x - corners[1].x));
	double height = fmax(fabs(corners[2].y - corners[0].y), fabs(corners[3].y - corners[1].y));
	if (a == VuoAnchor_make(VuoHorizontalAlignment_Left, VuoVerticalAlignment_Top))
		return (VuoPoint2d){
			corners[0].x,
			-corners[0].y
		};
	else if (a == VuoAnchor_make(VuoHorizontalAlignment_Left, VuoVerticalAlignment_Center))
		return (VuoPoint2d){
			(corners[0].x + corners[3].x)/2,
			-((corners[0].y + corners[3].y)/2 + height/2.)
		};
	else if (a == VuoAnchor_make(VuoHorizontalAlignment_Left, VuoVerticalAlignment_Bottom))
		return (VuoPoint2d){
			corners[3].x,
			-(corners[3].y + height)
		};
	else if (a == VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Top))
		return (VuoPoint2d){
			(corners[0].x + corners[1].x)/2 + width/2.,
			-(corners[0].y + corners[1].y)/2
		};
	else if (a == VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Bottom))
		return (VuoPoint2d){
			(corners[2].x + corners[3].x)/2 + width/2.,
			-((corners[2].y + corners[3].y)/2 + height)
		};
	else if (a == VuoAnchor_make(VuoHorizontalAlignment_Right, VuoVerticalAlignment_Top))
		return (VuoPoint2d){
			corners[1].x + width,
			-corners[1].y
		};
	else if (a == VuoAnchor_make(VuoHorizontalAlignment_Right, VuoVerticalAlignment_Center))
		return (VuoPoint2d){
			(corners[1].x + corners[2].x)/2 + width,
			-((corners[1].y + corners[2].y)/2 + height/2.)
		};
	else //if (a == VuoAnchor_make(VuoHorizontalAlignment_Right, VuoVerticalAlignment_Bottom))
		return (VuoPoint2d){
			corners[2].x + width,
			-(corners[2].y + height)
		};
}
