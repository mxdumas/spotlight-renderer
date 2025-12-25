cbuffer MatrixBuffer : register(b0) {
    matrix world;
    matrix view;
    matrix projection;
    matrix invViewProj;
    float4 cameraPos;
};

struct SpotlightData {
    matrix lightViewProj;
    float4 posRange;      // xyz: pos, w: range
    float4 dirAngle;      // xyz: dir, w: spotAngle
    float4 colorInt;      // xyz: color, w: intensity
    float4 coneGobo;      // x: beam, y: field, z: rotation
    float4 goboOff;       // xy: offset
};

#define MAX_LIGHTS 4

cbuffer SpotlightBuffer : register(b1) {
    SpotlightData lights[MAX_LIGHTS];
};

cbuffer VolumetricBuffer : register(b2) {
    float4 volParams; // x: stepCount, y: density, z: intensity, w: anisotropy (G)
    float4 volJitter; // x: time
};

Texture2D depthTexture : register(t0);
Texture2D goboTexture : register(t1);
Texture2D shadowMap : register(t2);

SamplerState samLinear : register(s0);
SamplerComparisonState shadowSampler : register(s1);

struct VS_INPUT {
    float3 pos : POSITION;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f);
    return output;
}

float3 ScreenToWorld(float2 uv, float depth) {
    float4 clipPos = float4(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPos = mul(clipPos, invViewProj);
    return worldPos.xyz / worldPos.w;
}

float HenyeyGreenstein(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0f - g2) / (4.0f * 3.14159f * pow(max(0.001f, 1.0f + g2 - 2.0f * g * cosTheta), 1.5f));
}

// Interleaved Gradient Noise - deterministic, spatially coherent dither pattern
// Avoids temporal jitter while preventing banding
float InterleavedGradientNoise(float2 screenPos) {
    float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
    return frac(magic.z * frac(dot(screenPos, magic.xy)));
}

float4 PS(PS_INPUT input) : SV_Target {
    float2 uv = input.pos.xy / float2(1920.0f, 1080.0f);

    uint2 pixelPos = uint2(input.pos.xy);
    float depth = depthTexture.Load(uint3(pixelPos, 0)).r;

    float3 worldPos = ScreenToWorld(uv, depth);
    float3 camPos = cameraPos.xyz;

    if (depth >= 1.0f) {
        worldPos = ScreenToWorld(uv, 1.0f);
    }

    float3 rayDirVec = worldPos - camPos;
    float rayLen = length(rayDirVec);
    float3 rayDir = rayDirVec / max(rayLen, 0.0001f);

    float g = volParams.w;
    float stepCount = volParams.x;
    float3 accumulatedLight = float3(0, 0, 0);

    float noise = frac(sin(dot(uv + volJitter.x, float2(12.9898, 78.233))) * 43758.5453);

    // To optimize, we find the global t_min/t_max that covers ALL active lights.
    // Ideally we would march per light, but that's expensive.
    // Instead, we march the full ray length (up to max range of any light) or just geometry depth.
    // For simplicity: March from 0 to rayLen.
    
    float t_min = 0.0f;
    float t_max = rayLen;
    float marchDist = t_max - t_min;
    float stepLen = marchDist / max(stepCount, 1.0f);
    float3 stepVec = rayDir * stepLen;

    float3 rayStart = camPos + rayDir * t_min;
    float3 currentPos = rayStart + stepVec * noise;

    // Optimization: Pre-calculate light vectors for active lights? 
    // No, we need to do it per step.

    for (int s = 0; s < (int)stepCount; ++s) {
        
        float3 stepContribution = float3(0,0,0);

        [unroll]
        for (int i = 0; i < MAX_LIGHTS; ++i) {
            // Check if light is active (intensity > 0)
            if (lights[i].colorInt.w <= 0.0f) continue;

            float3 LPos = lights[i].posRange.xyz;
            float range = lights[i].posRange.w;

            float3 toLight = LPos - currentPos;
            float dist = length(toLight);
            
            if (dist < range) {
                float attenuation = lights[i].colorInt.w / (dist * dist + 1.0f);
                float3 LDir = normalize(lights[i].dirAngle.xyz);
                float beam = lights[i].coneGobo.x;
                float field = lights[i].coneGobo.y;
                float3 toLightNorm = toLight / max(dist, 0.0001f);

                float cosAngle = dot(-toLightNorm, LDir);
                float spotEffect = saturate((cosAngle - field) / (max(0.001f, beam - field)));

                if (spotEffect > 0) {
                    float shadow = 1.0f;
                    
                    // Shadow mapping only for the first light for now
                    if (i == 0) {
                        float4 lightSpacePos = mul(float4(currentPos, 1.0f), lights[i].lightViewProj);
                        if (lightSpacePos.w > 0.0f) {
                            float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
                            float2 shadowUV = projCoords.xy * 0.5f + 0.5f;
                            shadowUV.y = 1.0f - shadowUV.y;

                            if (shadowUV.x >= 0 && shadowUV.x <= 1 && shadowUV.y >= 0 && shadowUV.y <= 1) {
                                shadow = shadowMap.SampleCmpLevelZero(shadowSampler, shadowUV, projCoords.z - 0.01f).r;
                            }
                        }
                    }

                    if (shadow > 0) {
                        float cosTheta = dot(rayDir, -toLightNorm);
                        float phase = HenyeyGreenstein(cosTheta, g);

                        // Gobo sampling
                        float3 goboColor = float3(1,1,1);
                        // Project texture
                        float4 lightSpacePos = mul(float4(currentPos, 1.0f), lights[i].lightViewProj);
                         if (lightSpacePos.w > 0.0f) {
                            float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
                             // Gobo rotation
                            float s, c;
                            sincos(lights[i].coneGobo.z, s, c);
                            float2 rUV;
                            rUV.x = projCoords.x * c - projCoords.y * s;
                            rUV.y = projCoords.x * s + projCoords.y * c;
                            rUV += lights[i].goboOff.xy;
                            float2 goboUV = rUV * 0.5f + 0.5f;
                            goboUV.y = 1.0f - goboUV.y;
                            
                            // Using Clamp to avoid bleeding
                            // Ensure gobo is only inside the cone effectively
                            if (goboUV.x >= 0 && goboUV.x <= 1 && goboUV.y >= 0 && goboUV.y <= 1)
                                goboColor = goboTexture.SampleLevel(samLinear, goboUV, 0).rgb;
                            else
                                goboColor = float3(0,0,0);
                         }

                        stepContribution += lights[i].colorInt.xyz * attenuation * spotEffect * shadow * goboColor * phase;
                    }
                }
            }
        }
        
        accumulatedLight += stepContribution * volParams.y * stepLen;
        currentPos += stepVec;
    }

    return float4(accumulatedLight * volParams.z, 1.0f);
}