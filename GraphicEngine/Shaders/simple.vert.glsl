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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColour;
layout(location = 0) out vec4 worldPosition;
layout(location = 1) out vec4 fragColour;
layout(location = 2) out vec4 normal;

void main()
{
	worldPosition = worldMatrix * vec4(inPosition, 1.0);

	gl_Position = projMatrix * viewMatrix * worldPosition;

	normal = normalize(vec4(inPosition, 0.0));
	fragColour = vec4(inColour, 1.0);
}