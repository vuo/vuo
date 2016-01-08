/**
 * @file
 * VuoShader shader definitions.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

/**
 * Projects `position`, and provides an unprojected `position`, used for triangulation.
 */
static const char *defaultVertexShaderSourceForGeometryShader = VUOSHADER_GLSL_SOURCE(120,
	// Inputs provided by VuoSceneRenderer
	uniform mat4 projectionMatrix;
	uniform mat4 modelviewMatrix;
	attribute vec4 position;

	// Outputs to geometry shader
	varying vec4 positionForGeometry;

	void main()
	{
		positionForGeometry = modelviewMatrix * position;
		gl_Position = projectionMatrix * positionForGeometry;
	}
);

/**
 * Helper for @ref VuoShader_makeDefaultShader.
 */
static VuoShader VuoShader_makeDefaultShaderInternal(void)
{
	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, include(trianglePoint));
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, include(triangleLine));

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		// Inputs
		varying vec4 fragmentTextureCoordinate;

		void main()
		{
			// Based on the Gritz/Baldwin antialiased checkerboard shader.

			vec3 color0 = vec3(1   -fragmentTextureCoordinate.x, fragmentTextureCoordinate.y,      1);
			vec3 color1 = vec3(0.75-fragmentTextureCoordinate.x, fragmentTextureCoordinate.y-0.25, 0.75);
			float frequency = 8;
			vec2 filterWidth = fwidth(fragmentTextureCoordinate.xy) * frequency;

			vec2 checkPos = fract(fragmentTextureCoordinate.xy * frequency);
			vec2 p = smoothstep(vec2(0.5), filterWidth + vec2(0.5), checkPos) +
				(1 - smoothstep(vec2(0),   filterWidth,             checkPos));

			gl_FragColor = vec4(mix(color0, color1, p.x*p.y + (1-p.x)*(1-p.y)), 1);
		}
	);

	const char *fragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
		// Inputs
		varying vec4 fragmentTextureCoordinate;
		varying vec4 vertexPosition;
		varying mat3 vertexPlaneToWorld;

		void main()
		{
			vertexPosition;
			vertexPlaneToWorld;

			// Based on the Gritz/Baldwin antialiased checkerboard shader.

			vec3 color0 = vec3(1   -fragmentTextureCoordinate.x, fragmentTextureCoordinate.y,      1);
			vec3 color1 = vec3(0.75-fragmentTextureCoordinate.x, fragmentTextureCoordinate.y-0.25, 0.75);
			float frequency = 8;
			vec2 filterWidth = fwidth(fragmentTextureCoordinate.xy) * frequency;

			vec2 checkPos = fract(fragmentTextureCoordinate.xy * frequency);
			vec2 p = smoothstep(vec2(0.5), filterWidth + vec2(0.5), checkPos) +
					(1 - smoothstep(vec2(0),   filterWidth,             checkPos));

			gl_FragColor = vec4(mix(color0, color1, p.x*p.y + (1-p.x)*(1-p.y)), 1);
		}
	);

	VuoShader shader = VuoShader_make("Default Shader (Checkerboard)");

	VuoShader_addSource                      (shader, VuoMesh_Points,              defaultVertexShaderSourceForGeometryShader, pointGeometryShaderSource, fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     defaultVertexShaderSourceForGeometryShader, lineGeometryShaderSource,  fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, NULL,                                       NULL,                      fragmentShaderSource);

	return shader;
}

/**
 * Returns a shared instance of the default (unlit checkerboard) shader.
 * It's a gradient checkerboard (white in the top-left corner),
 * so you can see the object and get a feel for its texture coordinates.
 *
 * @threadAny
 */
VuoShader VuoShader_makeDefaultShader(void)
{
	static dispatch_once_t once;
	static VuoShader defaultShader;
	dispatch_once(&once, ^{
					  defaultShader = VuoShader_makeDefaultShaderInternal();
					  VuoRegisterSingleton(defaultShader);
					  VuoRegisterSingleton(defaultShader->name);
					  VuoRegisterSingleton(defaultShader->pointProgram.vertexSource);
					  VuoRegisterSingleton(defaultShader->pointProgram.geometrySource);
					  VuoRegisterSingleton(defaultShader->pointProgram.fragmentSource);
					  VuoRegisterSingleton(defaultShader->lineProgram.vertexSource);
					  VuoRegisterSingleton(defaultShader->lineProgram.geometrySource);
					  VuoRegisterSingleton(defaultShader->lineProgram.fragmentSource);
					  VuoRegisterSingleton(defaultShader->triangleProgram.vertexSource);
					  VuoRegisterSingleton(defaultShader->triangleProgram.fragmentSource);
				  });
	return defaultShader;
}

/**
 * Returns a shader that renders objects with an image (ignoring lighting).
 *
 * @c image must be @c GL_TEXTURE_2D.
 *
 * @threadAny
 */
VuoShader VuoShader_makeUnlitImageShader(VuoImage image, VuoReal alpha)
{
	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, include(trianglePoint));
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, include(triangleLine));

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		// Inputs from ports
		uniform sampler2D texture;
		uniform float alpha;

		// Inputs from vertex/geometry shader
		varying vec4 fragmentTextureCoordinate;

		void main()
		{
			vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
			color.a = min(color.a, 1.);	// clamp alpha at 1 (for floating-point textures)
			color.rgb /= color.a;	// un-premultiply
			color.a *= alpha;
			if (color.a < 1./255.)
				discard;
			gl_FragColor = color;
		}
	);

	const char *fragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
		// Inputs from ports
		uniform sampler2D texture;
		uniform float alpha;

		// Inputs from vertex/geometry shader
		varying vec4 vertexPosition;
		varying mat3 vertexPlaneToWorld;
		varying vec4 fragmentTextureCoordinate;

		void main()
		{
			vertexPosition;
			vertexPlaneToWorld;

			vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
			color.a = min(color.a, 1.);	// clamp alpha at 1 (for floating-point textures)
			color.rgb /= color.a;	// un-premultiply
			color.a *= alpha;
			if (color.a < 1./255.)
				discard;
			gl_FragColor = color;
		}
	);

	VuoShader shader = VuoShader_make("Image Shader (Unlit)");

	VuoShader_addSource                      (shader, VuoMesh_Points,              defaultVertexShaderSourceForGeometryShader, pointGeometryShaderSource, fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     defaultVertexShaderSourceForGeometryShader, lineGeometryShaderSource,  fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, NULL,                                       NULL,                      fragmentShaderSource);

	VuoShader_setUniform_VuoImage(shader, "texture", image);
	VuoShader_setUniform_VuoReal (shader, "alpha",   alpha);

	return shader;
}

/**
 * Returns a shader that renders objects with an image (ignoring lighting).
 * Alpha values are passed through unmodified.
 *
 * @c image must be @c GL_TEXTURE_2D.
 *
 * @threadAny
 */
VuoShader VuoShader_makeUnlitAlphaPassthruImageShader(VuoImage image)
{
	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		// Inputs from ports
		uniform sampler2D texture;
		uniform float alpha;

		// Inputs from vertex/geometry shader
		varying vec4 fragmentTextureCoordinate;

		void main()
		{
			gl_FragColor = texture2D(texture, fragmentTextureCoordinate.xy);
		}
	);

	VuoShader shader = VuoShader_make("Image Shader (Unlit, AlphaPassthru)");
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoImage(shader, "texture", image);
	return shader;
}

/**
 * Returns a shader that renders objects with an image (ignoring lighting).
 *
 * @c image must be @c GL_TEXTURE_RECTANGLE_ARB.
 *
 * @threadAny
 */
VuoShader VuoShader_makeGlTextureRectangleShader(VuoImage image, VuoReal alpha)
{
	if (!image)
		return NULL;

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		// Inputs
		uniform sampler2DRect texture;
		uniform vec2 textureSize;
		uniform float alpha;
		varying vec4 fragmentTextureCoordinate;

		void main()
		{
			vec4 color = texture2DRect(texture, fragmentTextureCoordinate.xy*textureSize);
			color.a = min(color.a, 1.);	// clamp alpha at 1 (for floating-point textures)
			color.rgb /= color.a;	// un-premultiply
			color.a *= alpha;
			if (color.a < 1./255.)
				discard;
			gl_FragColor = color;
		}
	);

	VuoShader shader = VuoShader_make("Image Shader (GL_TEXTURE_RECTANGLE)");
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoImage  (shader, "texture",     image);
	VuoShader_setUniform_VuoReal   (shader, "alpha",       alpha);
	VuoShader_setUniform_VuoPoint2d(shader, "textureSize", VuoPoint2d_make(image->pixelsWide, image->pixelsHigh));
	return shader;
}

/**
 * Returns a shader that renders objects with an image (ignoring lighting).
 * Alpha values are passed through unmodified.
 *
 * @c image must be @c GL_TEXTURE_RECTANGLE_ARB.
 *
 * @threadAny
 */
VuoShader VuoShader_makeGlTextureRectangleAlphaPassthruShader(VuoImage image)
{
	if (!image)
		return NULL;

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		// Inputs
		uniform sampler2DRect texture;
		uniform vec2 textureSize;
		uniform float alpha;
		varying vec4 fragmentTextureCoordinate;

		void main()
		{
			gl_FragColor = texture2DRect(texture, fragmentTextureCoordinate.xy*textureSize);
		}
	);

	VuoShader shader = VuoShader_make("Image Shader (GL_TEXTURE_RECTANGLE, AlphaPassthru)");
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoImage  (shader, "texture",     image);
	VuoShader_setUniform_VuoPoint2d(shader, "textureSize", VuoPoint2d_make(image->pixelsWide, image->pixelsHigh));
	return shader;
}

/**
 * Returns a shader that renders a solid @c color.
 *
 * @threadAny
 */
VuoShader VuoShader_makeUnlitColorShader(VuoColor color)
{
	const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
		// Inputs from VuoSceneRenderer
		uniform mat4 projectionMatrix;
		uniform mat4 modelviewMatrix;
		attribute vec4 position;

		void main()
		{
			gl_Position = projectionMatrix * modelviewMatrix * position;
		}
	);

	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, include(trianglePoint));
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, include(triangleLine));

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		// Inputs from ports
		uniform vec4 color;

		void main()
		{
			gl_FragColor = color;
		}
	);

	const char *fragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
		// Inputs from ports
		uniform vec4 color;

		// Inputs from vertex/geometry shader
		varying vec4 vertexPosition;
		varying mat3 vertexPlaneToWorld;
		varying vec4 fragmentTextureCoordinate;

		void main()
		{
			vertexPosition;
			vertexPlaneToWorld;
			fragmentTextureCoordinate;

			gl_FragColor = color;
		}
	);

	VuoShader shader = VuoShader_make("Color Shader (Unlit)");

	VuoShader_addSource                      (shader, VuoMesh_Points,              defaultVertexShaderSourceForGeometryShader, pointGeometryShaderSource, fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     defaultVertexShaderSourceForGeometryShader, lineGeometryShaderSource,  fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, vertexShaderSource,                         NULL,                      fragmentShaderSource);

	VuoShader_setUniform_VuoColor(shader, "color", color);

	return shader;
}

/**
 * Returns a shader that renders a solid @c color circle.
 *
 * When sharpness = 1, the circle takes up half the size of the texture coordinates
 * (circumscribing a rectangle from (0.25,0.25) to (0.75,0.75)).
 *
 * When sharpness = 0, the circle's edge is blurred to take up the entire texture coordinate area.
 *
 * @threadAny
 */
VuoShader VuoShader_makeUnlitCircleShader(VuoColor color, VuoReal sharpness)
{
	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		uniform vec4 color;
		uniform float sharpness;
		varying vec4 fragmentTextureCoordinate;

		void main(void)
		{
			float dist = distance(fragmentTextureCoordinate.xy, vec2(0.5,0.5));
			float delta = fwidth(dist);
			gl_FragColor = mix(color, vec4(color.rgb,0), smoothstep(sharpness/2 - delta, 1 - sharpness/2 + delta, dist*2));
		}
	);

	VuoShader shader = VuoShader_make("Circle Shader");
	shader->objectScale = 0.5;
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoColor(shader, "color", color);
	VuoShader_setUniform_VuoReal (shader, "sharpness", VuoReal_clamp(sharpness, 0, 1));
	return shader;
}

/**
 * Returns a shader that renders a solid @c color rounded rectangle.
 *
 * When sharpness = 1, the rounded rectangle takes up half the size of the texture coordinates
 * (from (0.25,0.25) to (0.75,0.75)).
 *
 * When sharpness = 0, the rounded rectangle's edge is blurred to take up the entire texture coordinate area.
 *
 * `aspect` specifies the aspect ratio of the rectangle.  `aspect < 1` causes artifacts; instead, reciprocate the aspect and rotate the geometry 90°.
 *
 * @threadAny
 */
VuoShader VuoShader_makeUnlitRoundedRectangleShader(VuoColor color, VuoReal sharpness, VuoReal roundness, VuoReal aspect)
{
	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		uniform vec4 color;
		uniform float sharpness;
		uniform float roundness;
		uniform float aspect;
		varying vec4 fragmentTextureCoordinate;

		void main(void)
		{
			float roundness2 = min(aspect, roundness);
			roundness2 = max(1. - sharpness, roundness2);
			float diameter = roundness2 / 2.;
			diameter = max(diameter, 0.001);
			float radius = diameter / 2.;
			vec2 r = vec2(radius/aspect, radius);

			vec2 cornerCircleCenter = vec2(0.,0.);
			if (fragmentTextureCoordinate.x > 0.75 - r.x)
			{
				if (fragmentTextureCoordinate.y > 0.75 - r.y)
					// Top right corner
					cornerCircleCenter = vec2(0.75, 0.75) + vec2(-r.x, -r.y);
				else if (fragmentTextureCoordinate.y < 0.25 + r.y)
					// Bottom right corner
					cornerCircleCenter = vec2(0.75, 0.25) + vec2(-r.x,  r.y);
			}
			else if (fragmentTextureCoordinate.x < 0.25 + r.x)
			{
				if (fragmentTextureCoordinate.y > 0.75 - r.y)
					// Top left corner
					cornerCircleCenter = vec2(0.25, 0.75) + vec2( r.x, -r.y);
				else if (fragmentTextureCoordinate.y < 0.25 + radius)
					// Bottom left corner
					cornerCircleCenter = vec2(0.25, 0.25) + vec2( r.x,  r.y);
			}

			float dist = 0.;
			if (cornerCircleCenter.x > 0.)
				dist = distance((fragmentTextureCoordinate.xy - cornerCircleCenter) * vec2(aspect, 1.) + cornerCircleCenter, cornerCircleCenter) / diameter;
			else
			{
				float n = (fragmentTextureCoordinate.x - 0.5) / aspect * (aspect * (1. - (1. - sharpness) * (1. - sharpness)));

				if (fragmentTextureCoordinate.y < 0.5 + n)
				{
					if (fragmentTextureCoordinate.y > 0.5 - n)
						// Right edge
						dist = (fragmentTextureCoordinate.x - (0.75 - r.x)) * aspect;
					else
						// Bottom edge
						dist = (0.25 + r.y) - fragmentTextureCoordinate.y;
				}
				else
				{
					if (fragmentTextureCoordinate.y > 0.5 - n)
						// Top edge
						dist = fragmentTextureCoordinate.y - (0.75 - r.y);
					else
						// Left edge
						dist = ((0.25 + r.x) - fragmentTextureCoordinate.x) * aspect;
				}

				dist /= diameter;
			}

			float delta = fwidth(dist) / 2.;

			// fwidth() seems to sometimes output invalid results; this hides most of it.
			if ((roundness2 < 0.1 && delta > 3.)
			 || (roundness2 > 0.1 && delta > 0.1))
				delta = 0.;

			gl_FragColor = mix(color, vec4(color.rgb, 0.), smoothstep(sharpness / 2. - delta, 1. - sharpness / 2. + delta, dist));
		}
	);

	VuoShader shader = VuoShader_make("Rounded Rectangle Shader");
	shader->objectScale = 0.5;
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoColor(shader, "color",     color);
	VuoShader_setUniform_VuoReal (shader, "sharpness", VuoReal_clamp(sharpness, 0, 1));
	VuoShader_setUniform_VuoReal (shader, "roundness", VuoReal_clamp(roundness, 0, 1));
	VuoShader_setUniform_VuoReal (shader, "aspect",    aspect);
	return shader;
}

/**
 * Returns a shader that renders a color with lighting.
 *
 * @param diffuseColor The primary material color.
 * @param highlightColor The color of shiny specular highlights. Alpha controls the intensity of the highlights.
 * @param shininess A number representing how shiny the material is.  0 = dull; 1 = shiny; numbers in between represent varying amounts of shininess.
 *
 * @threadAny
 */
VuoShader VuoShader_makeLitColorShader(VuoColor diffuseColor, VuoColor highlightColor, VuoReal shininess)
{
	const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
		// Inputs provided by VuoSceneRenderer
		uniform mat4 projectionMatrix;
		uniform mat4 modelviewMatrix;
		attribute vec4 position;
		attribute vec4 normal;
		attribute vec4 tangent;
		attribute vec4 bitangent;

		// Outputs to fragment shader
		varying vec4 vertexPosition;
		varying mat3 vertexPlaneToWorld;
		varying vec4 fragmentTextureCoordinate;

		void main()
		{
			fragmentTextureCoordinate;

			vertexPosition = modelviewMatrix * position;

			vertexPlaneToWorld[0] = normalize(vec3(modelviewMatrix *  vec4(tangent.xyz,   0.)));
			vertexPlaneToWorld[1] = normalize(vec3(modelviewMatrix * -vec4(bitangent.xyz, 0.)));
			vertexPlaneToWorld[2] = normalize(vec3(modelviewMatrix *  vec4(normal.xyz,    0.)));

			gl_Position = projectionMatrix * vertexPosition;
		}
	);

	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, include(trianglePoint));
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, include(triangleLine));

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		include(lighting)

		// Inputs from ports
		uniform vec4 diffuseColor;
		uniform vec4 specularColor;
		uniform float specularPower;

		// Inputs from vertex/geometry shader
		varying vec4 fragmentTextureCoordinate;

		void main()
		{
			fragmentTextureCoordinate;

			vec3 ambientContribution = vec3(0.);
			vec3 diffuseContribution = vec3(0.);
			vec3 specularContribution = vec3(0.);

			calculateLighting(specularPower, vec3(0,0,1), ambientContribution, diffuseContribution, specularContribution);

			ambientContribution *= diffuseColor.rgb;
			diffuseContribution *= diffuseColor.rgb;
			specularContribution *= specularColor.rgb * specularColor.a;
			gl_FragColor = vec4(ambientContribution + diffuseContribution + specularContribution, diffuseColor.a);
		}
	);

	VuoShader shader = VuoShader_make("Color Shader (Lit)");

	VuoShader_addSource                      (shader, VuoMesh_Points,              defaultVertexShaderSourceForGeometryShader, pointGeometryShaderSource, fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     defaultVertexShaderSourceForGeometryShader, lineGeometryShaderSource,  fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, vertexShaderSource,                         NULL,                      fragmentShaderSource);

	VuoShader_setUniform_VuoColor(shader, "diffuseColor", diffuseColor);
	VuoShader_setUniform_VuoColor(shader, "specularColor", highlightColor);
	VuoShader_setUniform_VuoReal (shader, "specularPower", 1./(1.0001-shininess));
	return shader;
}

/**
 * A linear-projection vertex shader.
 * Also builds a matrix that transforms between world coordinates and coordinates on a plane tangent to the surface.
 */
static const char *lightingVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs provided by VuoSceneRenderer
	uniform mat4 projectionMatrix;
	uniform mat4 modelviewMatrix;
	attribute vec4 position;
	attribute vec4 normal;
	attribute vec4 tangent;
	attribute vec4 bitangent;
	attribute vec4 textureCoordinate;

	// Outputs to fragment shader
	varying vec4 vertexPosition;
	varying vec4 fragmentTextureCoordinate;
	varying mat3 vertexPlaneToWorld;

	void main()
	{
		vertexPosition = modelviewMatrix * position;

		fragmentTextureCoordinate = textureCoordinate;

		vertexPlaneToWorld[0] = normalize(vec3(modelviewMatrix * vec4(tangent.xyz,0.)));
		vertexPlaneToWorld[1] = normalize(vec3(modelviewMatrix * -vec4(bitangent.xyz,0.)));
		vertexPlaneToWorld[2] = normalize(vec3(modelviewMatrix * vec4(normal.xyz,0.)));

		gl_Position = projectionMatrix * vertexPosition;
	}
);

/**
 * A linear-projection vertex shader.
 * Also builds a matrix that transforms between world coordinates and coordinates on a plane tangent to the surface.
 */
static const char *lightingVertexShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
	// Inputs provided by VuoSceneRenderer
	uniform mat4 projectionMatrix;
	uniform mat4 modelviewMatrix;
	attribute vec4 position;
	attribute vec4 normal;
	attribute vec4 tangent;
	attribute vec4 bitangent;
	attribute vec4 textureCoordinate;

	// Outputs to geometry shader
	varying vec4 positionForGeometry;

	void main()
	{
		positionForGeometry = modelviewMatrix * position;
		gl_Position = projectionMatrix * positionForGeometry;
	}
);

/**
 * Returns a shader that renders an image with lighting.
 *
 * @param image The image which provides the diffuse / primary material color.
 * @param alpha The opacity of the image (0 to 1).
 * @param highlightColor The color of shiny specular highlights. Alpha controls the intensity of the highlights.
 * @param shininess A number representing how shiny the material is.  0 = dull; 1 = shiny; numbers in between represent varying amounts of shininess.
 *
 * @threadAny
 */
VuoShader VuoShader_makeLitImageShader(VuoImage image, VuoReal alpha, VuoColor highlightColor, VuoReal shininess)
{
	if (!image)
		return NULL;

	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, include(trianglePoint));
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, include(triangleLine));

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		include(lighting)

		// Inputs from vertex shader
		varying vec4 fragmentTextureCoordinate;

		// Inputs from ports
		uniform sampler2D texture;
		uniform float alpha;
		uniform vec4 specularColor;
		uniform float specularPower;

		void main()
		{
			vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
			color.a = min(color.a, 1.);	// clamp alpha at 1 (for floating-point textures)
			color.rgb /= color.a;	// un-premultiply
			color.a *= alpha;
			if (color.a < 1./255.)
				discard;

			vec3 ambientContribution = vec3(0.);
			vec3 diffuseContribution = vec3(0.);
			vec3 specularContribution = vec3(0.);

			calculateLighting(specularPower, vec3(0,0,1), ambientContribution, diffuseContribution, specularContribution);

			ambientContribution *= color.rgb;
			diffuseContribution *= color.rgb;
			specularContribution *= specularColor.rgb * specularColor.a;
			gl_FragColor = vec4(ambientContribution + diffuseContribution + specularContribution, color.a);
		}
	);

	VuoShader shader = VuoShader_make("Image Shader (Lit)");

	VuoShader_addSource                      (shader, VuoMesh_Points,              lightingVertexShaderSourceForGeometry, pointGeometryShaderSource, fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     lightingVertexShaderSourceForGeometry, lineGeometryShaderSource,  fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, lightingVertexShaderSource,			  NULL,                      fragmentShaderSource);

	VuoShader_setUniform_VuoImage(shader, "texture",       image);
	VuoShader_setUniform_VuoReal (shader, "alpha",         alpha);
	VuoShader_setUniform_VuoColor(shader, "specularColor", highlightColor);
	VuoShader_setUniform_VuoReal (shader, "specularPower", 1./(1.0001-shininess));

	return shader;
}

/**
 * Returns a shader that renders an image with lighting and surface details.
 *
 * @param image The image which provides the diffuse / primary material color.
 * @param alpha The opacity of the image (0 to 1).
 * @param specularImage An image that specifies the specular color (RGB) and shininess (A).
 * @param normalImage An image that specifies the surface details.  The red and green channels respectively define the normal direction along the tangent and bitangent axes (0 = negative; 0.5 = straight; 1 = positive).  The blue channel defines the height along the normal axis (0 = low; 1 = high).
 *
 * @threadAny
 */
VuoShader VuoShader_makeLitImageDetailsShader(VuoImage image, VuoReal alpha, VuoImage specularImage, VuoImage normalImage)
{
	if (!image || !specularImage || !normalImage)
		return NULL;

	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, include(trianglePoint));
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, include(triangleLine));

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		include(lighting)

		// Inputs from vertex shader
		varying vec4 fragmentTextureCoordinate;

		// Inputs from ports
		uniform sampler2D texture;
		uniform float alpha;
		uniform sampler2D specularImage;
		uniform sampler2D normalImage;

		void main()
		{
			vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
			color.a = min(color.a, 1.);	// clamp alpha at 1 (for floating-point textures)
			color.rgb /= color.a;	// un-premultiply
			color.a *= alpha;
			if (color.a < 1./255.)
				discard;

			vec3 ambientContribution = vec3(0.);
			vec3 diffuseContribution = vec3(0.);
			vec3 specularContribution = vec3(0.);

			vec4 specularColor = texture2D(specularImage, fragmentTextureCoordinate.xy);
			float specularPower = 1./(1.0001-specularColor.a);

			vec4 normalColor = texture2D(normalImage, fragmentTextureCoordinate.xy);
			vec3 normal = normalize(vec3(
						2. * normalColor.r - 1.,
						2. * normalColor.g - 1.,
						normalColor.b	// Leave the blue channel as-is; the normal should never point inward.
						));

			calculateLighting(specularPower, normal, ambientContribution, diffuseContribution, specularContribution);

			ambientContribution *= color.rgb;
			diffuseContribution *= color.rgb;
			specularContribution *= specularColor.rgb * specularColor.a;
			gl_FragColor = vec4(ambientContribution + diffuseContribution + specularContribution, color.a);
		}
	);

	VuoShader shader = VuoShader_make("Image Details Shader (Lit)");

	VuoShader_addSource                      (shader, VuoMesh_Points,              lightingVertexShaderSourceForGeometry, pointGeometryShaderSource, fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     lightingVertexShaderSourceForGeometry, lineGeometryShaderSource,  fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, lightingVertexShaderSource,			  NULL,                      fragmentShaderSource);

	VuoShader_setUniform_VuoImage(shader, "texture",       image);
	VuoShader_setUniform_VuoReal (shader, "alpha",         alpha);
	VuoShader_setUniform_VuoImage(shader, "specularImage", specularImage);
	VuoShader_setUniform_VuoImage(shader, "normalImage",   normalImage);

	return shader;
}

/**
 * Returns a shader that renders a linear gradient using the provided colors and start and end coordinates.
 * Coordinates should be passed in Vuo scene coordinates (-1,-1) to (1,1).
 *
 * @threadAny
 */
VuoShader VuoShader_makeLinearGradientShader(VuoList_VuoColor colors, VuoPoint2d start, VuoPoint2d end)
{
	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		uniform float gradientCount;
		uniform sampler2D gradientStrip;
		uniform vec2 start;
		uniform vec2 end;
		varying vec4 fragmentTextureCoordinate;

		float distSqr(vec2 a, vec2 b)
		{
			return (b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y);
		}

		// https://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment
		// ...minus the segment part, of course.
		vec2 nearest_point_on_line(vec2 v, vec2 w, vec2 p)
		{
			// Return minimum distance between line segment vw and point p
			float l2 = distSqr(v, w);  // i.e. |w-v|^2 -  avoid a sqrt
			if (l2 == 0.0) return p;
			// Consider the line extending the segment, parameterized as v + t (w - v).
			// We find projection of point p onto the line.
			// It falls where t = [(p-v) . (w-v)] / |w-v|^2
			float t = dot(p - v, w - v) / l2;
			vec2 projection = v + t * (w - v);
			return projection;
		}

		void main(void)
		{
			vec2 pol = nearest_point_on_line(start, end, fragmentTextureCoordinate.xy);
			float x = dot(pol-start, end-start) > 0 ? distance(start, pol)/ distance(start, end) : 0;
			float gradientWidth = (1./gradientCount)/2.;
			x = x * (1-gradientWidth*2) + gradientWidth;	// scale to account for the gradient/2 offsets
			vec4 color = texture2D(gradientStrip, vec2(clamp(x , gradientWidth, 1.-gradientWidth), .5));
			color.a = min(color.a, 1.);	// clamp alpha at 1 (for floating-point textures)
			color.rgb /= color.a;	// un-premultiply
			gl_FragColor = color;
		}
	);

	int len = VuoListGetCount_VuoColor(colors);

	unsigned char* pixels = (unsigned char*)malloc(sizeof(char)*len*4);
	int n = 0;
	for(int i = 1; i <= len; i++)
	{
		VuoColor col = VuoListGetValue_VuoColor(colors, i);
		pixels[n++] = (unsigned int)(col.a*col.r*255);
		pixels[n++] = (unsigned int)(col.a*col.g*255);
		pixels[n++] = (unsigned int)(col.a*col.b*255);
		pixels[n++] = (unsigned int)(col.a*255);
	}

	VuoImage gradientStrip = VuoImage_makeFromBuffer(pixels, GL_RGBA, len, 1, VuoImageColorDepth_8);

	VuoShader shader = VuoShader_make("Linear Gradient Shader");
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoImage  (shader, "gradientStrip", gradientStrip);
	VuoShader_setUniform_VuoReal   (shader, "gradientCount", len);
	VuoShader_setUniform_VuoPoint2d(shader, "start", VuoPoint2d_make((start.x+1)/2, (start.y+1)/2));
	VuoShader_setUniform_VuoPoint2d(shader, "end", VuoPoint2d_make((end.x+1)/2, (end.y+1)/2));
	return shader;
}

/**
 * Returns a shader that renders a radial gradient using the provided colors, center point, and radius.
 * Center and radius are expected in Vuo scene coordinates.  Width and Height may be either pixels or scene coordinates, as they
 * are only used to calculate the aspect ratio.
 *
 * @threadAny
 */
VuoShader VuoShader_makeRadialGradientShader(VuoList_VuoColor colors, VuoPoint2d center, VuoReal radius, VuoReal width, VuoReal height)
{
	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		uniform float gradientCount;
		uniform sampler2D gradientStrip;
		uniform vec2 center;
		uniform vec2 scale;	// if image is not square, multiply texCoord by this to account for stretch
		uniform float radius;
		varying vec4 fragmentTextureCoordinate;

		void main(void)
		{
			vec2 scaledTexCoord = fragmentTextureCoordinate.xy*scale;
			float x = distance(center*scale, scaledTexCoord)/radius;
			float gradientWidth = (1./gradientCount)/2.;
			x = x * (1-gradientWidth*2) + gradientWidth;
			vec4 color = texture2D(gradientStrip, vec2(clamp(x , gradientWidth, 1.-gradientWidth), .5));
			color.a = min(color.a, 1.);	// clamp alpha at 1 (for floating-point textures)
			color.rgb /= color.a;	// un-premultiply
			gl_FragColor = color;
		}
	);

	// VuoPoint2d scale = width < height ? VuoPoint2d_make(1., height/(float)width) : VuoPoint2d_make(width/(float)height, 1.);
	VuoPoint2d scale = VuoPoint2d_make(1., height/(float)width);

	// todo Is this used enough to warrant it's own function?
	int len = VuoListGetCount_VuoColor(colors);
	unsigned char* pixels = (unsigned char*)malloc(sizeof(char)*len*4);
	int n = 0;
	for(int i = 1; i <= len; i++)
	{
		VuoColor col = VuoListGetValue_VuoColor(colors, i);
		pixels[n++] = (unsigned int)(col.a*col.r*255);
		pixels[n++] = (unsigned int)(col.a*col.g*255);
		pixels[n++] = (unsigned int)(col.a*col.b*255);
		pixels[n++] = (unsigned int)(col.a*255);
	}
	VuoImage gradientStrip = VuoImage_makeFromBuffer(pixels, GL_RGBA, len, 1, VuoImageColorDepth_8);

	VuoShader shader = VuoShader_make("Radial Gradient Shader");
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoImage  (shader, "gradientStrip", gradientStrip);
	VuoShader_setUniform_VuoReal   (shader, "gradientCount", len);
	VuoShader_setUniform_VuoPoint2d(shader, "center", VuoPoint2d_make((center.x+1)/2, (center.y+1)/2));
	VuoShader_setUniform_VuoReal   (shader, "radius", radius > 0. ? radius/2. : 0);
	VuoShader_setUniform_VuoPoint2d(shader, "scale",  VuoPoint2d_make(scale.x, scale.y));
	return shader;
}
