// Defines the uniforms passed by VuoSceneRenderer, the varyings provided by @c lightingVertexShaderSource, and the @c calculateLighting() function.

uniform mat4 modelviewMatrix;

uniform vec3 cameraPosition;

uniform vec4 ambientColor;
uniform float ambientBrightness;

struct PointLight
{
	vec4 color;
	float brightness;
	vec3 position;
	float range;
	float sharpness;
};
uniform PointLight pointLights[16];
uniform int pointLightCount;

struct SpotLight
{
	vec4 color;
	float brightness;
	vec3 position;
	vec3 direction;
	float cone;
	float range;
	float sharpness;
};
uniform SpotLight spotLights[16];
uniform int spotLightCount;

varying vec4 vertexPosition;
varying mat3 vertexPlaneToWorld;

void calculateLighting(
		in float specularPower,
		in vec3 normal,
		out vec3 ambientContribution,
		out vec3 diffuseContribution,
		out vec3 specularContribution
	)
{
	ambientContribution = ambientColor.rgb * ambientColor.a * ambientBrightness;
	diffuseContribution = specularContribution = vec3(0.);

	vec3 normalDirection = normalize(vertexPlaneToWorld * normal) * (gl_FrontFacing?1:-1);

	int i;
	for (i=0; i<pointLightCount; ++i)
	{
		float lightDistance = distance(pointLights[i].position, vertexPosition.xyz);
		float range = pointLights[i].range;
		float sharpness = pointLights[i].sharpness;
		float lightRangeFactor = 1. - smoothstep(range*sharpness, range*(2-sharpness), lightDistance);

		vec3 scaledLightColor = lightRangeFactor * pointLights[i].color.rgb * pointLights[i].color.a * pointLights[i].brightness;

		vec3 incidentLightDirection = normalize(pointLights[i].position - vertexPosition.xyz);
		diffuseContribution += scaledLightColor * max(dot(normalDirection, incidentLightDirection), 0.);

		vec3 reflection = reflect(-incidentLightDirection, normalDirection);
		vec3 cameraDirection = normalize(cameraPosition.xyz - vertexPosition.xyz);
		specularContribution += scaledLightColor * pow(max(dot(reflection, cameraDirection), 0.), specularPower);
	}

	for (i=0; i<spotLightCount; ++i)
	{
		float lightDistance = distance(spotLights[i].position, vertexPosition.xyz);
		float range = spotLights[i].range;
		float sharpness = spotLights[i].sharpness;
		float lightRangeFactor = 1. - smoothstep(range*sharpness, range*(2-sharpness), lightDistance);

		vec3 incidentLightDirection = normalize(spotLights[i].position - vertexPosition.xyz);
		float cosSpotDirection = dot(-incidentLightDirection, spotLights[i].direction);
		float cone = spotLights[i].cone;
		float lightDirectionFactor = smoothstep(cos(cone*(2-sharpness)/2), cos(cone*sharpness/2), cosSpotDirection);

		vec3 scaledLightColor = lightRangeFactor * lightDirectionFactor * spotLights[i].color.rgb * spotLights[i].color.a * spotLights[i].brightness;

		diffuseContribution += scaledLightColor * max(dot(normalDirection, incidentLightDirection), 0.);

		vec3 reflection = reflect(-incidentLightDirection, normalDirection);
		vec3 cameraDirection = normalize(cameraPosition.xyz - vertexPosition.xyz);
		specularContribution += scaledLightColor * pow(max(dot(reflection, cameraDirection), 0.), specularPower);
	}
}
