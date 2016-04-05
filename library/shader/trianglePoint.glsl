/**
 * A geometry shader, to be executed once for each point.
 *
 * Input: The vertex of the point.
 *
 * Output: A pair of triangles (6 vertices), to render the point as a solid quad.
 */

// Inputs from VuoSceneRenderer
uniform float aspectRatio;
uniform float primitiveHalfSize;
uniform bool hasTextureCoordinates;

// Inputs from vertex shader
varying in vec4 positionForGeometry[1];
varying in vec4 textureCoordinateForGeometry[1];

// Outputs to fragment shader
varying out vec4 vertexPosition;
varying out mat3 vertexPlaneToWorld;
varying out vec4 fragmentTextureCoordinate;

void main()
{
	vec2 pointSize = vec2(primitiveHalfSize, primitiveHalfSize * aspectRatio);

	mat3 vertexPlaneToWorldTemp;
	vertexPlaneToWorldTemp[0] =  vec3(1,0,0); // tangent
	vertexPlaneToWorldTemp[1] = -vec3(0,1,0); // bitangent
	vertexPlaneToWorldTemp[2] =  vec3(0,0,1); // normal

	gl_Position               = gl_PositionIn[0]       + vec4(-pointSize.x,  pointSize.y, 0, 0);
	vertexPosition            = positionForGeometry[0] + vec4(-pointSize.x,  pointSize.y, 0, 0);
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = hasTextureCoordinates ? textureCoordinateForGeometry[0] : vec4(0,1,0,0);
	EmitVertex();
	gl_Position               = gl_PositionIn[0]       + vec4(-pointSize.x, -pointSize.y, 0, 0);
	vertexPosition            = positionForGeometry[0] + vec4(-pointSize.x, -pointSize.y, 0, 0);
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = hasTextureCoordinates ? textureCoordinateForGeometry[0] : vec4(0,0,0,0);
	EmitVertex();
	gl_Position               = gl_PositionIn[0]       + vec4( pointSize.x, -pointSize.y, 0, 0);
	vertexPosition            = positionForGeometry[0] + vec4( pointSize.x, -pointSize.y, 0, 0);
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = hasTextureCoordinates ? textureCoordinateForGeometry[0] : vec4(1,0,0,0);
	EmitVertex();
	EndPrimitive();

	gl_Position               = gl_PositionIn[0]       + vec4( pointSize.x, -pointSize.y, 0, 0);
	vertexPosition            = positionForGeometry[0] + vec4( pointSize.x, -pointSize.y, 0, 0);
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = hasTextureCoordinates ? textureCoordinateForGeometry[0] : vec4(1,0,0,0);
	EmitVertex();
	gl_Position               = gl_PositionIn[0]       + vec4( pointSize.x,  pointSize.y, 0, 0);
	vertexPosition            = positionForGeometry[0] + vec4( pointSize.x,  pointSize.y, 0, 0);
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = hasTextureCoordinates ? textureCoordinateForGeometry[0] : vec4(1,1,0,0);
	EmitVertex();
	gl_Position               = gl_PositionIn[0]       + vec4(-pointSize.x,  pointSize.y, 0, 0);
	vertexPosition            = positionForGeometry[0] + vec4(-pointSize.x,  pointSize.y, 0, 0);
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = hasTextureCoordinates ? textureCoordinateForGeometry[0] : vec4(0,1,0,0);
	EmitVertex();
	EndPrimitive();
}
