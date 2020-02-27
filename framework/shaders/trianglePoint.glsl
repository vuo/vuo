/**
 * A geometry shader, to be executed once for each point.
 *
 * Input: The vertex of the point.
 *
 * Output: A pair of triangles (6 vertices), to render the point as a solid quad.
 */

// Inputs from VuoSceneRenderer
uniform mat4 modelviewMatrix;
uniform float aspectRatio;
uniform float primitiveHalfSize;
uniform bool hasTextureCoordinates;

// Inputs from vertex shader
varying in vec3 geometryPosition[1];
varying in vec3 geometryNormal[1];
varying in vec2 geometryTextureCoordinate[1];
varying in vec4 geometryVertexColor[1];

// Outputs to fragment shader
varying out vec3 fragmentPosition;
varying out vec3 fragmentNormal;
varying out vec2 fragmentTextureCoordinate;
varying out vec4 fragmentVertexColor;
varying out mat3 vertexPlaneToWorld;

void main()
{
	vec2 pointSize = vec2(primitiveHalfSize, primitiveHalfSize * aspectRatio);

	gl_Position               = gl_PositionIn[0]    + vec4(-pointSize.x,  pointSize.y, 0, 0);
	fragmentPosition          = geometryPosition[0] + vec3(-pointSize.x,  pointSize.y, 0);
	fragmentNormal            = geometryNormal[0];
	fragmentTextureCoordinate = hasTextureCoordinates ? geometryTextureCoordinate[0] : vec2(0,1);
	fragmentVertexColor       = geometryVertexColor[0];
	vertexPlaneToWorld[0]     = normalize(vec3(modelviewMatrix *  vec4(1., 0., 0., 0.)));
	vertexPlaneToWorld[1]     = normalize(vec3(modelviewMatrix * -vec4(0., 1., 0., 0.)));
	vertexPlaneToWorld[2]     = normalize(vec3(modelviewMatrix *  vec4(geometryNormal[0], 0.)));
	EmitVertex();

	gl_Position               = gl_PositionIn[0]    + vec4(-pointSize.x, -pointSize.y, 0, 0);
	fragmentPosition          = geometryPosition[0] + vec3(-pointSize.x, -pointSize.y, 0);
	fragmentNormal            = geometryNormal[0];
	fragmentTextureCoordinate = hasTextureCoordinates ? geometryTextureCoordinate[0] : vec2(0,0);
	fragmentVertexColor       = geometryVertexColor[0];
	EmitVertex();

	gl_Position               = gl_PositionIn[0]    + vec4( pointSize.x, -pointSize.y, 0, 0);
	fragmentPosition          = geometryPosition[0] + vec3( pointSize.x, -pointSize.y, 0);
	fragmentNormal            = geometryNormal[0];
	fragmentTextureCoordinate = hasTextureCoordinates ? geometryTextureCoordinate[0] : vec2(1,0);
	fragmentVertexColor       = geometryVertexColor[0];
	EmitVertex();
	EndPrimitive();

	gl_Position               = gl_PositionIn[0]    + vec4( pointSize.x, -pointSize.y, 0, 0);
	fragmentPosition          = geometryPosition[0] + vec3( pointSize.x, -pointSize.y, 0);
	fragmentNormal            = geometryNormal[0];
	fragmentTextureCoordinate = hasTextureCoordinates ? geometryTextureCoordinate[0] : vec2(1,0);
	fragmentVertexColor       = geometryVertexColor[0];
	EmitVertex();

	gl_Position               = gl_PositionIn[0]    + vec4( pointSize.x,  pointSize.y, 0, 0);
	fragmentPosition          = geometryPosition[0] + vec3( pointSize.x,  pointSize.y, 0);
	fragmentNormal            = geometryNormal[0];
	fragmentTextureCoordinate = hasTextureCoordinates ? geometryTextureCoordinate[0] : vec2(1,1);
	fragmentVertexColor       = geometryVertexColor[0];
	EmitVertex();

	gl_Position               = gl_PositionIn[0]    + vec4(-pointSize.x,  pointSize.y, 0, 0);
	fragmentPosition          = geometryPosition[0] + vec3(-pointSize.x,  pointSize.y, 0);
	fragmentNormal            = geometryNormal[0];
	fragmentTextureCoordinate = hasTextureCoordinates ? geometryTextureCoordinate[0] : vec2(0,1);
	fragmentVertexColor       = geometryVertexColor[0];
	EmitVertex();
	EndPrimitive();
}
