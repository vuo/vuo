/**
 * A geometry shader, to be executed once for each line segment.
 *
 * Input: The 2 vertices of the line segment.
 *
 * Output: A pair of triangles (6 vertices), to render the line segment as a solid quad.
 */

// Inputs from VuoSceneRenderer
uniform mat4 projectionMatrix;
uniform float aspectRatio;
uniform float primitiveHalfSize;
uniform vec3 cameraPosition;
uniform bool hasTextureCoordinates;
uniform bool useFisheyeProjection;

// Inputs from vertex shader
varying in vec4 positionForGeometry[2];
varying in vec4 textureCoordinateForGeometry[2];

// Outputs to fragment shader
varying out vec4 vertexPosition;
varying out mat3 vertexPlaneToWorld;
varying out vec4 fragmentTextureCoordinate;

void main()
{
	vec2 lineSize = vec2(primitiveHalfSize, primitiveHalfSize * aspectRatio);

	vec3 cameraPosition = useFisheyeProjection ? vec3(0,0,-1000) : (projectionMatrix * vec4(cameraPosition,1)).xyz;

	// Screen-space direction perpendicular to the line segment.
	vec3 perpendicular = normalize(
				cross(gl_PositionIn[1].xyz - gl_PositionIn[0].xyz,
					  gl_PositionIn[0].xyz - cameraPosition)
			);

	vec4 perpendicularOffset = vec4(perpendicular.x*lineSize.x, perpendicular.y*lineSize.y, 0, 0);

	mat3 vertexPlaneToWorldTemp;
	vertexPlaneToWorldTemp[0] =  vec3(1,0,0); // tangent
	vertexPlaneToWorldTemp[1] = -vec3(0,1,0); // bitangent
	vertexPlaneToWorldTemp[2] =  vec3(0,0,1); // normal

	gl_Position               = gl_PositionIn[1]       - perpendicularOffset;
	vertexPosition            = positionForGeometry[1] - perpendicularOffset;
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = hasTextureCoordinates ? textureCoordinateForGeometry[1] : vec4(0,1,0,0);
	EmitVertex();
	gl_Position               = gl_PositionIn[0]       - perpendicularOffset;
	vertexPosition            = positionForGeometry[0] - perpendicularOffset;
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = hasTextureCoordinates ? textureCoordinateForGeometry[0] : vec4(0,0,0,0);
	EmitVertex();
	gl_Position               = gl_PositionIn[0]       + perpendicularOffset;
	vertexPosition            = positionForGeometry[0] + perpendicularOffset;
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = hasTextureCoordinates ? textureCoordinateForGeometry[0] : vec4(1,0,0,0);
	EmitVertex();
	EndPrimitive();

	gl_Position               = gl_PositionIn[0]       + perpendicularOffset;
	vertexPosition            = positionForGeometry[0] + perpendicularOffset;
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = hasTextureCoordinates ? textureCoordinateForGeometry[0] : vec4(1,0,0,0);
	EmitVertex();
	gl_Position               = gl_PositionIn[1]       + perpendicularOffset;
	vertexPosition            = positionForGeometry[1] + perpendicularOffset;
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = hasTextureCoordinates ? textureCoordinateForGeometry[1] : vec4(1,1,0,0);
	EmitVertex();
	gl_Position               = gl_PositionIn[1]       - perpendicularOffset;
	vertexPosition            = positionForGeometry[1] - perpendicularOffset;
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = hasTextureCoordinates ? textureCoordinateForGeometry[1] : vec4(0,1,0,0);
	EmitVertex();
	EndPrimitive();
}
