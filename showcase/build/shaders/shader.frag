#version 450
layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 2, binding = 0) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosWorld;
layout(location = 3) in vec3 fragNormalWorld;
layout(location = 4) in vec3 cameraPosWorld;
layout(location = 5) in vec4 inShadowCoord;

layout(location = 0) out vec4 outColor;

struct PointLight {
    vec3 position;
    vec4 color; // w intensity
    vec3 args; // x constant, y linear, z quadratic
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;

    vec3 directionalLight;
    vec4 directionalLightColor;
    mat4 lightSpace;
    
    int lightNum;
    PointLight pointLights[4];
} ubo;

layout(push_constant) uniform Push {
    mat4 model;
    mat4 normal;
    vec3 color;
    vec3 ambient;
    vec3 diffuse;
    vec4 specular;
    int args;
} push;

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( shadowMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = 0.2;
		}
	}
	return shadow;
}

// 获得泊松圆盘样本点
vec2 poissonDisk(float radius, vec2 center, float seed)
{
    float angle = 2.0 * 3.14159265359 * seed;
    float r = sqrt(seed);
    return center + vec2(r * cos(angle), r * sin(angle)) * radius;
}

float filterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(shadowMap, 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;

  int numSamples = 8;  // 调整采样点数量
  float radius = 1.0;   // 调整泊松圆盘的半径
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
      // 分割每个网格并在子区域中应用泊松圆盘采样
      for (int i = 0; i < numSamples; i++)
      {
          vec2 sampleOffset = vec2(dx, dy) * poissonDisk(radius, vec2(float(x) + 0.5, float(y) + 0.5), float(i) / float(numSamples));
          shadowFactor += textureProj(sc, sampleOffset);
      }
      count += numSamples;
		}
	
	}
	return shadowFactor / count;
}

float fog(float density)
{
	const float l2 = -1.442695;
	float dist = gl_FragCoord.z / gl_FragCoord.w * 0.1;
	float d = density * dist;
	return 1.0 - clamp(exp2(d * d * l2), 0.0, 1.0);
}


vec3 calcDiffuse(vec3 intensity, vec3 normal, vec3 lightDir)
{
  float diff = max(dot(normal, lightDir), 0.0);
  return push.diffuse * diff * intensity;
}

vec3 calcSpecular(vec3 intensity, vec3 normal, vec3 lightDir, vec3 halfwayDir)
{
  // specular lighting
  float blinnTerm = dot(normal, halfwayDir);
  blinnTerm = clamp(blinnTerm, 0, 1);
  blinnTerm = pow(blinnTerm, push.specular.a); // higher values -> sharper highlight
  return intensity * blinnTerm * push.specular.rgb;
}

void main() {
    // outColor = push.useTexture ? texture(texSampler, fragTexCoord) : vec4(fragColor, 1.0);
    // 测试光源
    // PointLight light;
    // light.position = vec3(5.0, 5.0, 5.0);
    // light.color = vec4(1.0, 1.0, 1.0, 1.0);
    // light.constant = 1.0;
    // light.linear = 0.09;
    // light.quadratic = 0.032;

    // Apply texture if needed
    vec4 textureColor = texture(texSampler, fragTexCoord);

    float shadow = ((push.args & 2) != 0) ? filterPCF(inShadowCoord / inShadowCoord.w) : 1.0f;

    if ((push.args & 1) != 0) {
        float ambientStrength = 1;
        vec3 tmpLighting = vec3(0.0);

        vec3 surfaceNormal = normalize(fragNormalWorld);
        vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

        vec3 lightDirection, halfwayDir, intensity, ambient, diffuse, specular;

         // 先计算 Directional light
        if ((push.args & 4) != 0) {
            lightDirection = -ubo.directionalLight;
            halfwayDir = normalize(lightDirection + viewDirection);
            intensity = ubo.directionalLightColor.rgb * ubo.directionalLightColor.a;

            ambient = ambientStrength * push.ambient;
            diffuse = calcDiffuse(intensity, surfaceNormal, lightDirection);
            specular = calcSpecular(intensity, surfaceNormal, lightDirection, halfwayDir);

            tmpLighting += (ambient + diffuse + specular);
        }

        for (int i = 0; i < ubo.lightNum; i++) {
            PointLight light = ubo.pointLights[i];
            lightDirection = normalize(light.position - fragPosWorld);
            halfwayDir = normalize(lightDirection + viewDirection);

            intensity = light.color.rgb * light.color.a;

            ambient = ambientStrength * push.ambient;
            diffuse = calcDiffuse(intensity, surfaceNormal, lightDirection);
            // specular lighting
            specular = calcSpecular(intensity, surfaceNormal, lightDirection, halfwayDir);

            // Attenuation
            float distance = length(light.position - fragPosWorld);
            float attenuation = 1.0 / (light.args.x + light.args.y * distance + light.args.z * (distance * distance));
            tmpLighting += (ambient + diffuse + specular) * attenuation;

            // 削弱shadow
            if (shadow < 1.0f) {
                shadow += attenuation * light.color.a;
                shadow = min(1.0f, shadow);
            }
        }
        vec3 finalColor = mix(tmpLighting * shadow, tmpLighting * 0.2, 1 - shadow) * push.color;
        // Combine
        outColor = textureColor * vec4(finalColor, 1.0);
    } else {
        outColor = textureColor * vec4(push.color * shadow, 1.0);
    }

    const vec4 fogColor = vec4(0.47, 0.5, 0.67, 0.0);
	  // 远处雾化
	outColor  = mix(outColor, fogColor, fog(0.4));	
}