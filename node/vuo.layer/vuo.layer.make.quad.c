/**
 * @file
 * vuo.layer.make.quad node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>
#include "VuoLayer.h"
#include "VuoEdgeBlend.h"
#include "VuoProjectionType.h"

VuoModuleMetadata({
					 "title" : "Make Quad Layer",
					 "keywords" : [ "perspective", "affine", "warp", "keystone", "quadrilateral", "trapezoid", "parallelogram",
									"mapping", "video mapping", "corner", "pin", "projector", "wall", "building", "architecture" ],
					 "version" : "1.0.0",
					 "dependencies" : [
					 ],
					 "node" : {
						  "exampleCompositions" : [ "DemoProjectionMappingCalibration.vuo" ]
					 }
				 });

static const char* fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	uniform sampler2D image;
	uniform vec2 scale;
	uniform vec2 offset;
	uniform vec4 fill;

	uniform float gamma;
	uniform vec4 edgeGammaCutoff;
	uniform vec4 edgeGammaPower;

	varying vec4 fragmentTextureCoordinate;

	vec4 applyGamma(vec4 c, float p)
	{
		return vec4( pow(c.x, p),
					 pow(c.y, p),
					 pow(c.z, p),
					 c.a);
	}

	void main(void)
	{
		vec2 uv = fragmentTextureCoordinate.xy / fragmentTextureCoordinate.w;
		vec4 color = texture2D(image, uv * scale + offset);
		color.rgb /= color.a > 0. ? color.a : 1.;

		float a = 0.;
		vec4 edgeGammaCutoffClamped = max(edgeGammaCutoff, vec4(0.));

		// left
		a = smoothstep(0., edgeGammaCutoffClamped.x, uv.x);
		color.a = mix(0., color.a, pow(a, edgeGammaPower.x));

		// right
		a = 1. - smoothstep(0.99999 - edgeGammaCutoffClamped.y, 1., uv.x);
		color.a = mix(0., color.a, pow(a, edgeGammaPower.y));

		// top
		a = 1. - smoothstep(0.99999 - edgeGammaCutoffClamped.z, 1., uv.y);
		color.a = mix(0., color.a, pow(a, edgeGammaPower.z));

		// bottom
		a = smoothstep(0., edgeGammaCutoffClamped.w, uv.y);
		color.a = mix(0., color.a, pow(a, edgeGammaPower.w));

		// overall gamma
		color = applyGamma(color, gamma);

		color.rgb *= color.a;
		gl_FragColor = color;
	}
);

/**
 * http://www.reedbeta.com/blog/2012/05/26/quadrilateral-interpolation-part-1/
 */
static void calculateHomogeneousCoordinates(const VuoPoint4d* positions, VuoPoint4d* textures)
{
	VuoPoint2d a = VuoPoint2d_make(	positions[3].x - positions[0].x,
									positions[3].y - positions[0].y);

	VuoPoint2d b = VuoPoint2d_make(	positions[2].x - positions[1].x,
									positions[2].y - positions[1].y);

	float cross = a.x * b.y - a.y * b.x;

	if(cross != 0.)
	{
		VuoPoint2d c = VuoPoint2d_make(	positions[0].x - positions[1].x,
										positions[0].y - positions[1].y);

		float s = (a.x * c.y - a.y * c.x) / cross;

		if(s > 0. && s < 1.)
		{
			float t = (b.x * c.y - b.y * c.x) / cross;

			if(t > 0 && t < 1)
			{
				float q0 = 1. / (1-t);
				float q1 = 1. / (1-s);
				float q2 = 1. / s;
				float q3 = 1. / t;

				textures[0] = VuoPoint4d_make(textures[0].x * q0, textures[0].y * q0, 0., q0);
				textures[1] = VuoPoint4d_make(textures[1].x * q1, textures[1].y * q1, 0., q1);
				textures[2] = VuoPoint4d_make(textures[2].x * q2, textures[2].y * q2, 0., q2);
				textures[3] = VuoPoint4d_make(textures[3].x * q3, textures[3].y * q3, 0., q3);
			}
		}
	}
}

#define lerp(x,y,a) ( VuoPoint2d_add(x, VuoPoint2d_multiply(VuoPoint2d_subtract(y,x), a)) )
#define make4d(xy, z, w) VuoPoint4d_make((xy).x, (xy).y, (z), (w))

static VuoSubmesh makeQuadSubmesh(const VuoPoint2d topLeft, const VuoPoint2d topRight, const VuoPoint2d bottomLeft, const VuoPoint2d bottomRight, const unsigned int subdivisions)
{
	unsigned int rows = subdivisions + 2;
	const unsigned int vertexCount = rows * rows;
	const unsigned int elementCount = (rows - 1) * (rows - 1) * 6;

	VuoPoint4d* m_positions 	= (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d* m_textures 		= (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	unsigned int* m_elements 	= (unsigned int*)malloc(sizeof(unsigned int) * elementCount);

	unsigned int index = 0;

	for(int i = 0; i < rows; i++)
	{
		float y = i/(float)(rows-1);

		VuoPoint2d left = lerp(bottomLeft, topLeft, y);
		VuoPoint2d right = lerp(bottomRight, topRight, y);

		for(int n = 0; n < rows; n++, index++)
		{
			float x = n / (float)(rows - 1);

			m_positions[index] = make4d(lerp(left, right, x), 0, 1);
			m_textures[index] = VuoPoint4d_make(x, y, 0, 1);
		}
	}

	index = 0;

	for(int i = 0; i < rows - 1; i++)
	{
		for(int n = 0; n < rows - 1; n++, index += 6)
		{
			int t0 = i * rows + n,
				t1 = t0 + 1,
				t2 = (i + 1) * rows + n,
				t3 = t2 + 1;

			m_elements[index + 0] = t2;
			m_elements[index + 1] = t0;
			m_elements[index + 2] = t1;

			m_elements[index + 3] = t1;
			m_elements[index + 4] = t3;
			m_elements[index + 5] = t2;
		}
	}

	if(vertexCount == 4)
		calculateHomogeneousCoordinates(m_positions, m_textures);

	VuoSubmesh submesh;

	submesh.vertexCount 			= vertexCount;
	submesh.elementCount			= elementCount;
	submesh.positions 				= m_positions;
	submesh.textureCoordinates 		= m_textures;
	submesh.elements				= m_elements;
	submesh.elementAssemblyMethod 	= VuoMesh_IndividualTriangles;
	submesh.normals 				= NULL;
	submesh.tangents				= NULL;
	submesh.bitangents				= NULL;
	submesh.faceCullingMode 		= GL_BACK;

	return submesh;
}

#undef lerp
#undef make4d

static VuoPoint2d perp(const VuoPoint2d p)
{
	return VuoPoint2d_make(-p.y, p.x);
}

static inline bool isLeft(VuoPoint2d p, VuoPoint2d line0, VuoPoint2d line1)
{
	VuoPoint2d cen = { (line0.x + line1.x) * .5, (line0.y + line1.y) * .5 };
	VuoPoint2d line = perp(VuoPoint2d_normalize(VuoPoint2d_subtract(line1, line0)));
	VuoPoint2d pdir = VuoPoint2d_normalize(VuoPoint2d_subtract(p, cen));
	return VuoPoint2d_dotProduct(line, pdir) > .001;
}

static bool validateControlPoints( VuoPoint2d tl, VuoPoint2d tr, VuoPoint2d bl, VuoPoint2d br )
{
	return	isLeft(tl, bl, tr) &&
			isLeft(tr, tl, br) &&
			isLeft(bl, br, tl) &&
			isLeft(br, tr, bl);
}

void nodeEvent
(
	VuoInputData(VuoImage) image,
	VuoInputData(VuoProjectionType, {"default": "perspective"} ) projection,
	VuoInputData(VuoReal, { "default":1, "suggestedMin":0.001, "suggestedMax":3, "suggestedStep":0.01}) gamma,
	VuoInputData(VuoPoint2d, {"default": {"x": -1.0, "y":  1.0 }, "suggestedMin": {"x":-1, "y":-1}, "suggestedMax": {"x":1, "y":1}, "suggestedStep": {"x":0.001, "y":0.001}}) topLeft,
	VuoInputData(VuoPoint2d, {"default": {"x":  1.0, "y":  1.0 }, "suggestedMin": {"x":-1, "y":-1}, "suggestedMax": {"x":1, "y":1}, "suggestedStep": {"x":0.001, "y":0.001}}) topRight,
	VuoInputData(VuoPoint2d, {"default": {"x": -1.0, "y": -1.0 }, "suggestedMin": {"x":-1, "y":-1}, "suggestedMax": {"x":1, "y":1}, "suggestedStep": {"x":0.001, "y":0.001}}) bottomLeft,
	VuoInputData(VuoPoint2d, {"default": {"x":  1.0, "y": -1.0 }, "suggestedMin": {"x":-1, "y":-1}, "suggestedMax": {"x":1, "y":1}, "suggestedStep": {"x":0.001, "y":0.001}}) bottomRight,
	VuoInputData(VuoEdgeBlend, { 	"default": {"cutoff":0, "gamma":1, "crop":0},
									"suggestedMin": {"cutoff":0, "gamma":0.001, "crop":0},
									"suggestedMax": {"cutoff":0.5, "gamma":3, "crop":0.5},
									"suggestedStep": {"cutoff":0.01, "gamma":0.1, "crop":0.01} })
									leftEdge,
	VuoInputData(VuoEdgeBlend, { 	"default": {"cutoff":0, "gamma":1, "crop":0},
									"suggestedMin": {"cutoff":0, "gamma":0.001, "crop":0},
									"suggestedMax": {"cutoff":0.5, "gamma":3, "crop":0.5},
									"suggestedStep": {"cutoff":0.01, "gamma":0.1, "crop":0.01} })
									rightEdge,
	VuoInputData(VuoEdgeBlend, { 	"default": {"cutoff":0, "gamma":1, "crop":0},
									"suggestedMin": {"cutoff":0, "gamma":0.001, "crop":0},
									"suggestedMax": {"cutoff":0.5, "gamma":3, "crop":0.5},
									"suggestedStep": {"cutoff":0.01, "gamma":0.1, "crop":0.01} })
									topEdge,
	VuoInputData(VuoEdgeBlend, { 	"default": {"cutoff":0, "gamma":1, "crop":0},
									"suggestedMin": {"cutoff":0, "gamma":0.001, "crop":0},
									"suggestedMax": {"cutoff":0.5, "gamma":3, "crop":0.5},
									"suggestedStep": {"cutoff":0.01, "gamma":0.1, "crop":0.01} })
									bottomEdge,
	VuoOutputData(VuoLayer) quad
)
{
	// if in perspective mode & the control points produce a concave mesh don't put out a broken layer
	if( projection == VuoProjectionType_Perspective && !validateControlPoints(topLeft, topRight, bottomLeft, bottomRight) )
	{
		/// @todo https://b33p.net/kosada/node/4724
		VUserLog("Attempting to create a perspective quad with invalid control points!  Either use \"Affine\" projection mapping or make sure that control points don't overlap.");
		*quad = VuoLayer_makeEmpty();
		return;
	}

	VuoSubmesh submesh = makeQuadSubmesh(topLeft, topRight, bottomLeft, bottomRight, projection == VuoProjectionType_Perspective ? 0 : 32);

	VuoMesh mesh = VuoMesh_makeFromSingleSubmesh(submesh);

	VuoShader unlitImagePerspDivide = VuoShader_make("Texture Perspective Divide");

	VuoShader_addSource(unlitImagePerspDivide, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);

	if(image != NULL)
	{
		float leftCrop = MAX(leftEdge.crop, 0);
		float rightCrop = MAX(rightEdge.crop, 0);
		float topCrop = MAX(topEdge.crop, 0);
		float bottomCrop = MAX(bottomEdge.crop, 0);

		VuoPoint2d scale = VuoPoint2d_make( 1 - (leftCrop + rightCrop), 1 - (topCrop + bottomCrop) );
		VuoPoint2d offset = VuoPoint2d_make(leftCrop, bottomCrop);

		VuoShader_setUniform_VuoImage(unlitImagePerspDivide, "image", image);
		VuoShader_setUniform_VuoPoint2d(unlitImagePerspDivide, "scale", scale);
		VuoShader_setUniform_VuoPoint2d(unlitImagePerspDivide, "offset", offset);
		VuoShader_setUniform_VuoReal(unlitImagePerspDivide, "gamma", gamma);
		VuoShader_setUniform_VuoPoint4d(unlitImagePerspDivide, "edgeGammaCutoff", VuoPoint4d_make(leftEdge.cutoff, rightEdge.cutoff, topEdge.cutoff, bottomEdge.cutoff));
		VuoShader_setUniform_VuoPoint4d(unlitImagePerspDivide, "edgeGammaPower", VuoPoint4d_make(leftEdge.gamma, rightEdge.gamma, topEdge.gamma, bottomEdge.gamma));
	}

	quad->sceneObject = VuoSceneObject_make(mesh,
		unlitImagePerspDivide,
		VuoTransform_makeIdentity(),
		NULL);
}
