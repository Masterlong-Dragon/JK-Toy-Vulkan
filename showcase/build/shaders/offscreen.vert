#version 450

layout (location = 0) in vec3 inPosition;

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 depthVP;
} ubo;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

layout(push_constant) uniform Push {
    mat4 model;
    mat4 normal;

    vec3 ambient;
    vec3 diffuse;
    vec4 specular;
    int useLighting;
} push;
 
void main()
{
	vec4 positionWorld = push.model * vec4(inPosition, 1.0);
	gl_Position =  ubo.depthVP * positionWorld;
}