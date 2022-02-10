/**
 * @file
 * vuo.layer.make.quad node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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
					 "version" : "1.1.1",
					 "dependencies" : [
					 ],
					 "node" : {
						  "exampleCompositions" : [ "DemoProjectionMappingCalibration.vuo" ]
					 }
				 });

static const char* fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	uniform sampler2D image;
	uniform vec2 scale;
	uniform vec2 offset;
	uniform vec4 fill;

	uniform float gamma;
	uniform float alpha;
	uniform vec4 edgeGammaCutoff;
	uniform vec4 edgeGammaPower;

	varying vec2 fragmentTextureCoordinate;
	varying vec4 fragmentVertexColor;

	vec4 applyGamma(vec4 c, float p)
	{
		return vec4( pow(c.x, p),
					 pow(c.y, p),
					 pow(c.z, p),
					 c.a);
	}

	void main(void)
	{
		float q = fragmentVertexColor.r;
		vec2 uv = fragmentTextureCoordinate / q;
		vec4 color = VuoGlsl_sample(image, uv * scale + offset);
		color *= alpha;
		VuoGlsl_discardInvisible(color.a);
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
static void calculateHomogeneousCoordinates(const float *positions, float *textureCoordinates, float *q)
{
	VuoPoint2d a = VuoPoint2d_make(positions[3 * 3    ] - positions[0 * 3    ],
								   positions[3 * 3 + 1] - positions[0 * 3 + 1]);

	VuoPoint2d b = VuoPoint2d_make(positions[2 * 3    ] - positions[1 * 3    ],
								   positions[2 * 3 + 1] - positions[1 * 3 + 1]);

	float cross = a.x * b.y - a.y * b.x;

	if(cross != 0.)
	{
		VuoPoint2d c = VuoPoint2d_make(positions[0 * 3    ] - positions[1 * 3    ],
									   positions[0 * 3 + 1] - positions[1 * 3 + 1]);

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

				textureCoordinates[0 * 2    ] *= q0;
				textureCoordinates[0 * 2 + 1] *= q0;
				textureCoordinates[1 * 2    ] *= q1;
				textureCoordinates[1 * 2 + 1] *= q1;
				textureCoordinates[2 * 2    ] *= q2;
				textureCoordinates[2 * 2 + 1] *= q2;
				textureCoordinates[3 * 2    ] *= q3;
				textureCoordinates[3 * 2 + 1] *= q3;
				q[0 * 4] = q0;
				q[1 * 4] = q1;
				q[2 * 4] = q2;
				q[3 * 4] = q3;
			}
		}
	}
}

#define lerp(x,y,a) ( VuoPoint2d_add(x, VuoPoint2d_multiply(VuoPoint2d_subtract(y,x), a)) )
#define make4d(xy, z, w) VuoPoint4d_make((xy).x, (xy).y, (z), (w))

static VuoMesh makeQuadMesh(const VuoPoint2d topLeft, const VuoPoint2d topRight, const VuoPoint2d bottomLeft, const VuoPoint2d bottomRight, const unsigned int subdivisions)
{
	unsigned int rows = subdivisions + 2;
	const unsigned int vertexCount = rows * rows;
	const unsigned int elementCount = (rows - 1) * (rows - 1) * 6;

	float *positions, *textureCoordinates, *q;
	unsigned int *elements;
	VuoMesh_allocateCPUBuffers(vertexCount, &positions, NULL, &textureCoordinates, &q, elementCount, &elements);

	unsigned int index = 0;

	for(int i = 0; i < rows; i++)
	{
		float y = i/(float)(rows-1);

		VuoPoint2d left = lerp(bottomLeft, topLeft, y);
		VuoPoint2d right = lerp(bottomRight, topRight, y);

		for(int n = 0; n < rows; n++, index++)
		{
			float x = n / (float)(rows - 1);

			positions[index * 3    ] = lerp(left, right, x).x;
			positions[index * 3 + 1] = lerp(left, right, x).y;
			positions[index * 3 + 2] = 0;
			textureCoordinates[index * 2    ] = x;
			textureCoordinates[index * 2 + 1] = y;
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

			elements[index + 0] = t2;
			elements[index + 1] = t0;
			elements[index + 2] = t1;

			elements[index + 3] = t1;
			elements[index + 4] = t3;
			elements[index + 5] = t2;
		}
	}

	if(vertexCount == 4)
		calculateHomogeneousCoordinates(positions, textureCoordinates, q);
	else
		for (int i = 0; i < vertexCount; ++i)
			q[i * 4] = 1;

	return VuoMesh_makeFromCPUBuffers(vertexCount,
		positions, NULL, textureCoordinates, q,
		elementCount, elements, VuoMesh_IndividualTriangles);
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

uint64_t nodeInstanceInit(void)
{
	return VuoSceneObject_getNextId();
}

void nodeInstanceEvent
(
	VuoInstanceData(uint64_t) id,
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
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) opacity,
	VuoOutputData(VuoLayer) quad
)
{
	// if in perspective mode & the control points produce a concave mesh don't put out a broken layer
	if( projection == VuoProjectionType_Perspective && !validateControlPoints(topLeft, topRight, bottomLeft, bottomRight) )
	{
		/// @todo https://b33p.net/kosada/node/4724
		VUserLog("Attempting to create a perspective quad with invalid control points!  Either use \"Affine\" projection mapping or make sure that control points don't overlap.");
		*quad = NULL;
		return;
	}

	VuoMesh mesh = makeQuadMesh(topLeft, topRight, bottomLeft, bottomRight, projection == VuoProjectionType_Perspective ? 0 : 32);

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
		VuoShader_setUniform_VuoReal(unlitImagePerspDivide, "alpha", opacity);
		VuoShader_setUniform_VuoPoint4d(unlitImagePerspDivide, "edgeGammaCutoff", VuoPoint4d_makeNonzero((VuoPoint4d){leftEdge.cutoff, rightEdge.cutoff, topEdge.cutoff, bottomEdge.cutoff}));
		VuoShader_setUniform_VuoPoint4d(unlitImagePerspDivide, "edgeGammaPower", VuoPoint4d_makeNonzero((VuoPoint4d){leftEdge.gamma, rightEdge.gamma, topEdge.gamma, bottomEdge.gamma}));
	}

	*quad = (VuoLayer)VuoSceneObject_makeMesh(mesh,
		unlitImagePerspDivide,
		VuoTransform_makeIdentity());
	VuoLayer_setId(*quad, *id);
}
