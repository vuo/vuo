void emitTriangle(vec3 position[3], vec4 textureCoordinate[3])
{
	vec3 ab = position[1] - position[0];
	vec3 ac = position[2] - position[0];
	vec4 normal    = vec4(normalize(cross(ab, ac)),1);
	vec4 tangent   = vec4(normalize(ab),1);
	vec4 bitangent = vec4(normalize(ac),1);

	for (int i=0; i<3; ++i)
	{
		outPosition = vec4(position[i],1);
		outNormal = normal;
		outTangent = tangent;
		outBitangent = bitangent;
		outTextureCoordinate = textureCoordinate[i];
		EmitVertex();
	}
	EndPrimitive();
}
