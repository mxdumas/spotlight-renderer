cbuffer MatrixBuffer : register(b0) {
    matrix world;
    matrix view;
    matrix projection;
    matrix invViewProj;
    float4 cameraPos;
};

cbuffer SpotlightBuffer : register(b1) {
    matrix lightViewProj : packoffset(c0);
    float4 posRange      : packoffset(c4); // xyz: pos, w: range
    float4 dirAngle      : packoffset(c5); // xyz: dir, w: spotAngle
    float4 colorInt      : packoffset(c6); // xyz: color, w: intensity
    float4 coneGobo      : packoffset(c7); // x: beam, y: field, z: rotation
    float4 goboOff       : packoffset(c8); // xy: offset
};

cbuffer VolumetricBuffer : register(b2) {
    float4 volParams; // x: stepCount, y: density, z: intensity, w: anisotropy (G)
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
    float4 screenPos : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.screenPos = output.pos;
    return output;
}

float3 ScreenToWorld(float2 screenUV, float depth) {
    float4 clipPos = float4(screenUV.x * 2.0f - 1.0f, (1.0f - screenUV.y) * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPos = mul(clipPos, invViewProj);
    return worldPos.xyz / worldPos.w;
}

float HenyeyGreenstein(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0f - g2) / (4.0f * 3.14159f * pow(max(0.001f, 1.0f + g2 - 2.0f * g * cosTheta), 1.5f));
}

float4 PS(PS_INPUT input) : SV_Target {
    float2 uv = input.screenPos.xy / input.screenPos.w * 0.5f + 0.5f;
    uv.y = 1.0f - uv.y;

    float depth = depthTexture.SampleLevel(samLinear, uv, 0).r;
    float3 worldPos = ScreenToWorld(uv, depth);
    float3 camPos = cameraPos.xyz;
    
    float3 rayDirVec = worldPos - camPos;
    float rayLen = length(rayDirVec);
    float3 rayDir = rayDirVec / rayLen;

    float3 rayStart = camPos;
    float stepCount = volParams.x;
    float stepLen = rayLen / stepCount;
    float3 stepVec = rayDir * stepLen;

    float3 accumulatedLight = float3(0, 0, 0);
    float3 currentPos = rayStart;

    // Spotlight params
    float3 LPos = posRange.xyz;
    float3 LDir = normalize(dirAngle.xyz);
    float beam = coneGobo.x;
    float field = coneGobo.y;
    float g = volParams.w;

    for (int i = 0; i < (int)stepCount; ++i) {
        float3 toLight = LPos - currentPos;
        float dist = length(toLight);
        float3 toLightNorm = toLight / dist;

        float attenuation = colorInt.w / (dist * dist + 1.0f);
        if (dist < posRange.w) {
            float cosAngle = dot(-toLightNorm, LDir);
            float spotEffect = saturate((cosAngle - field) / (max(0.001f, beam - field)));

            if (spotEffect > 0) {
                float shadow = 1.0f;
                float4 lightSpacePos = mul(float4(currentPos, 1.0f), lightViewProj);
                if (lightSpacePos.w > 0.0f) {
                    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
                    float2 shadowUV = projCoords.xy * 0.5f + 0.5f;
                    shadowUV.y = 1.0f - shadowUV.y;
                    
                    if (shadowUV.x >= 0 && shadowUV.x <= 1 && shadowUV.y >= 0 && shadowUV.y <= 1) {
                        shadow = shadowMap.SampleCmpLevelZero(shadowSampler, shadowUV, projCoords.z - 0.005f).r;
                    }

                    if (shadow > 0) {
                        // Phase function
                        float cosTheta = dot(rayDir, -toLightNorm);
                        float phase = HenyeyGreenstein(cosTheta, g);

                        // Gobo
                        float s, c;
                        sincos(coneGobo.z, s, c);
                        float2 rUV;
                        rUV.x = projCoords.x * c - projCoords.y * s;
                        rUV.y = projCoords.x * s + projCoords.y * c;
                        rUV += goboOff.xy;
                        float2 goboUV = rUV * 0.5f + 0.5f;
                        goboUV.y = 1.0f - goboUV.y;
                        float3 goboColor = goboTexture.SampleLevel(samLinear, goboUV, 0).rgb;

                        accumulatedLight += colorInt.xyz * attenuation * spotEffect * shadow * goboColor * phase * volParams.y;
                    }
                }
            }
        }
        currentPos += stepVec;
    }

    return float4(accumulatedLight * volParams.z, 1.0f);
}
