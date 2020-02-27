void emitTriangle(vec3 position[3], vec2 textureCoordinate[3], vec4 vertexColor[3])
{
	vec3 ab = position[1] - position[0];
	vec3 ac = position[2] - position[0];
	vec3 normal    = normalize(cross(ab, ac));
	vec3 tangent   = normalize(ab);
	vec3 bitangent = normalize(ac);

	for (int i=0; i<3; ++i)
	{
		outPosition = position[i];
		outNormal = normal;
		outTextureCoordinate = textureCoordinate[i];
		outVertexColor = vertexColor[i];
		EmitVertex();
	}
	EndPrimitive();
}
