// Inputs from VuoSceneRenderer
uniform float aspectRatio;
uniform float primitiveHalfSize;

// Inputs from vertex shader
varying in vec4 positionForGeometry[1];

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
	fragmentTextureCoordinate = vec4(0,1,0,0);
	EmitVertex();
	gl_Position               = gl_PositionIn[0]       + vec4(-pointSize.x, -pointSize.y, 0, 0);
	vertexPosition            = positionForGeometry[0] + vec4(-pointSize.x, -pointSize.y, 0, 0);
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = vec4(0,0,0,0);
	EmitVertex();
	gl_Position               = gl_PositionIn[0]       + vec4( pointSize.x, -pointSize.y, 0, 0);
	vertexPosition            = positionForGeometry[0] + vec4( pointSize.x, -pointSize.y, 0, 0);
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = vec4(1,0,0,0);
	EmitVertex();
	EndPrimitive();

	gl_Position               = gl_PositionIn[0]       + vec4( pointSize.x, -pointSize.y, 0, 0);
	vertexPosition            = positionForGeometry[0] + vec4( pointSize.x, -pointSize.y, 0, 0);
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = vec4(1,0,0,0);
	EmitVertex();
	gl_Position               = gl_PositionIn[0]       + vec4( pointSize.x,  pointSize.y, 0, 0);
	vertexPosition            = positionForGeometry[0] + vec4( pointSize.x,  pointSize.y, 0, 0);
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = vec4(1,1,0,0);
	EmitVertex();
	gl_Position               = gl_PositionIn[0]       + vec4(-pointSize.x,  pointSize.y, 0, 0);
	vertexPosition            = positionForGeometry[0] + vec4(-pointSize.x,  pointSize.y, 0, 0);
	vertexPlaneToWorld        = vertexPlaneToWorldTemp;
	fragmentTextureCoordinate = vec4(0,1,0,0);
	EmitVertex();
	EndPrimitive();
}
