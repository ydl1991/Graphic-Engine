#version 450

layout(binding = 0) uniform Uniforms
{
	mat4 viewMatrix;
	mat4 projMatrix;
	vec4 lightPosition;
	vec4 lightColor;
	vec4 cameraPosition;
};

layout(binding = 1) uniform ObjectUniforms
{
	mat4 worldMatrix;
	vec4 materialDiffuse;
	vec4 materialEmissive;
	vec4 materialAmbient;
	vec4 materialSpecular;
	float materialShininess;
	bool enableLighting;
};

layout(location = 0) in vec4 worldPosition;
layout(location = 1) in vec4 fragColour;
layout(location = 2) in vec4 normal;
layout(location = 0) out vec4 outColour;

void main()
{
	if (enableLighting)
	{
		vec4 globalAmbient = vec4(0.1f, 0.1f, 0.1f, 1.0f);

		// per-vertex lighting
		vec4 L = normalize(lightPosition - worldPosition);
		vec4 V = normalize(cameraPosition - worldPosition);
		vec4 H = normalize(L + V);
		float NdotL = dot(normal, L);
		float facing = NdotL;

		vec4 diffuse = lightColor * (max(NdotL, 0.0) * materialDiffuse);
		vec4 ambient = materialAmbient * globalAmbient * lightColor;
		vec4 emissive = materialEmissive;
		vec4 specular = (materialSpecular * facing * pow(max(dot(normal, H), 0), materialShininess)) * lightColor;

		outColour = diffuse + ambient + specular + emissive;
	}
	else
	{
		outColour = fragColour;
	}
}