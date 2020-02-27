/**
 * A geometry shader, to be executed once for each line segment.
 *
 * Input: The 2 vertices of the line segment.
 *
 * Output: A pair of triangles (6 vertices), to render the line segment as a solid quad.
 */

// Inputs from VuoSceneRenderer
uniform mat4 projectionMatrix;
uniform mat4 cameraMatrixInverse;
uniform mat4 modelviewMatrix;
uniform float aspectRatio;
uniform float primitiveHalfSize;
uniform vec3 cameraPosition;
uniform bool hasTextureCoordinates;
uniform bool useFisheyeProjection;

// Inputs from vertex shader
varying in vec3 geometryPosition[2];
varying in vec3 geometryNormal[2];
varying in vec2 geometryTextureCoordinate[2];
varying in vec4 geometryVertexColor[2];

// Outputs to fragment shader
varying out vec3 fragmentPosition;
varying out vec3 fragmentNormal;
varying out vec2 fragmentTextureCoordinate;
varying out vec4 fragmentVertexColor;
varying out mat3 vertexPlaneToWorld;

void main()
{
	vec2 lineSize = vec2(primitiveHalfSize, primitiveHalfSize * aspectRatio);

	vec3 cameraPosition = useFisheyeProjection ? vec3(0,0,-1000) : (projectionMatrix * cameraMatrixInverse * vec4(cameraPosition,1)).xyz;

	// Screen-space direction perpendicular to the line segment.
	vec3 perpendicular = normalize(
				cross(gl_PositionIn[1].xyz - gl_PositionIn[0].xyz,
					  gl_PositionIn[0].xyz - cameraPosition)
			);

	vec3 perpendicularOffset = vec3(perpendicular.x*lineSize.x, perpendicular.y*lineSize.y, 0);

	gl_Position               = gl_PositionIn[1]    - vec4(perpendicularOffset, 0.);
	fragmentPosition          = geometryPosition[1] - perpendicularOffset;
	fragmentNormal            = geometryNormal[1];
	fragmentTextureCoordinate = hasTextureCoordinates ? geometryTextureCoordinate[1] : vec2(0,1);
	fragmentVertexColor       = geometryVertexColor[1];
	vertexPlaneToWorld[0]     = normalize(vec3(modelviewMatrix *  vec4(1., 0., 0., 0.)));
	vertexPlaneToWorld[1]     = normalize(vec3(modelviewMatrix * -vec4(0., 1., 0., 0.)));
	vertexPlaneToWorld[2]     = normalize(vec3(modelviewMatrix *  vec4(geometryNormal[1], 0.)));
	EmitVertex();

	gl_Position               = gl_PositionIn[0]    - vec4(perpendicularOffset, 0.);
	fragmentPosition          = geometryPosition[0] - perpendicularOffset;
	fragmentNormal            = geometryNormal[0];
	fragmentTextureCoordinate = hasTextureCoordinates ? geometryTextureCoordinate[0] : vec2(0,0);
	fragmentVertexColor       = geometryVertexColor[0];
	vertexPlaneToWorld[2]     = normalize(vec3(modelviewMatrix *  vec4(geometryNormal[0], 0.)));
	EmitVertex();

	gl_Position               = gl_PositionIn[0]    + vec4(perpendicularOffset, 0.);
	fragmentPosition          = geometryPosition[0] + perpendicularOffset;
	fragmentNormal            = geometryNormal[0];
	fragmentTextureCoordinate = hasTextureCoordinates ? geometryTextureCoordinate[0] : vec2(1,0);
	fragmentVertexColor       = geometryVertexColor[0];
	vertexPlaneToWorld[2]     = normalize(vec3(modelviewMatrix *  vec4(geometryNormal[0], 0.)));
	EmitVertex();
	EndPrimitive();

	gl_Position               = gl_PositionIn[0]    + vec4(perpendicularOffset, 0.);
	fragmentPosition          = geometryPosition[0] + perpendicularOffset;
	fragmentNormal            = geometryNormal[0];
	fragmentTextureCoordinate = hasTextureCoordinates ? geometryTextureCoordinate[0] : vec2(1,0);
	fragmentVertexColor       = geometryVertexColor[0];
	vertexPlaneToWorld[2]     = normalize(vec3(modelviewMatrix *  vec4(geometryNormal[0], 0.)));
	EmitVertex();

	gl_Position               = gl_PositionIn[1]    + vec4(perpendicularOffset, 0.);
	fragmentPosition          = geometryPosition[1] + perpendicularOffset;
	fragmentNormal            = geometryNormal[1];
	fragmentTextureCoordinate = hasTextureCoordinates ? geometryTextureCoordinate[1] : vec2(1,1);
	fragmentVertexColor       = geometryVertexColor[1];
	vertexPlaneToWorld[2]     = normalize(vec3(modelviewMatrix *  vec4(geometryNormal[1], 0.)));
	EmitVertex();

	gl_Position               = gl_PositionIn[1]    - vec4(perpendicularOffset, 0.);
	fragmentPosition          = geometryPosition[1] - perpendicularOffset;
	fragmentNormal            = geometryNormal[1];
	fragmentTextureCoordinate = hasTextureCoordinates ? geometryTextureCoordinate[1] : vec2(0,1);
	fragmentVertexColor       = geometryVertexColor[1];
	vertexPlaneToWorld[2]     = normalize(vec3(modelviewMatrix *  vec4(geometryNormal[1], 0.)));
	EmitVertex();
	EndPrimitive();
}
