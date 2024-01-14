#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;

    vec3 directionalLight;
    vec4 directionalLightColor;
    mat4 lightSpace;
} ubo;

layout(std430, push_constant) uniform Push {
    mat4 model;
    mat4 normal;
} push;

const mat4 depth_bias = mat4( 
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0
);

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPosWorld;
layout(location = 3) out vec3 fragNormalWorld;
layout(location = 4) out vec3 cameraPosWorld;
layout(location = 5) out vec4 shadowCoord;

void main() {
    vec4 positionWorld = push.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * positionWorld;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragPosWorld = positionWorld.xyz;
    fragNormalWorld = normalize(mat3(push.normal) * inNormal);
    cameraPosWorld = inverse(ubo.view)[3].xyz;
    shadowCoord = depth_bias * ubo.lightSpace * positionWorld;
}