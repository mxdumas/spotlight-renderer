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
    float4 volJitter; // x: time, y: temporalWeight
    matrix prevViewProj;
};

Texture2D depthTexture : register(t0);
Texture2D goboTexture : register(t1);
Texture2D shadowMap : register(t2);
Texture2D historyTexture : register(t3);

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

float3 RGBToYCoCg(float3 rgb) {
    float y  = dot(rgb, float3(0.25f, 0.50f, 0.25f));
    float co = dot(rgb, float3(0.50f, 0.00f, -0.50f));
    float cg = dot(rgb, float3(-0.25f, 0.50f, -0.25f));
    return float3(y, co, cg);
}

float3 YCoCgToRGB(float3 ycocg) {
    float y  = ycocg.x;
    float co = ycocg.y;
    float cg = ycocg.z;
    float r = y + co - cg;
    float g = y + cg;
    float b = y - co - cg;
    return float3(r, g, b);
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

    float3 rayStart = camPos;
    float stepCount = volParams.x;
    float stepLen = rayLen / max(stepCount, 1.0f);
    float3 stepVec = rayDir * stepLen;

    float3 accumulatedLight = float3(0, 0, 0);
    
    // Original sine-based noise distribution
    float frameIndex = volJitter.x * 60.0f; 
    float2 noiseUV = uv + float2(frac(frameIndex * 0.61803398875f), frac(frameIndex * 0.70710678118f));
    float noise = frac(sin(dot(noiseUV, float2(12.9898f, 78.233f))) * 43758.5453f);
    
    float3 currentPos = rayStart + stepVec * noise;

    float3 LPos = posRange.xyz;
    float3 LDir = normalize(dirAngle.xyz);
    float beam = coneGobo.x;
    float field = coneGobo.y;
    float g = volParams.w;

    for (int i = 0; i < (int)stepCount; ++i) {
        float3 toLight = LPos - currentPos;
        float dist = length(toLight);
        float3 toLightNorm = toLight / max(dist, 0.0001f);

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
                        shadow = shadowMap.SampleCmpLevelZero(shadowSampler, shadowUV, projCoords.z - 0.001f).r;
                    }

                    if (shadow > 0) {
                        float cosTheta = dot(rayDir, -toLightNorm);
                        float phase = HenyeyGreenstein(cosTheta, g);

                        // Gobo sampling
                        float s, c;
                        sincos(coneGobo.z, s, c);
                        float2 rUV;
                        rUV.x = projCoords.x * c - projCoords.y * s;
                        rUV.y = projCoords.x * s + projCoords.y * c;
                        rUV += goboOff.xy;
                        float2 goboUV = rUV * 0.5f + 0.5f;
                        goboUV.y = 1.0f - goboUV.y;
                        float3 goboColor = goboTexture.SampleLevel(samLinear, goboUV, 0).rgb;

                        accumulatedLight += colorInt.xyz * attenuation * spotEffect * shadow * goboColor * phase * volParams.y * stepLen;
                    }
                }
            }
        }
        currentPos += stepVec;
    }

    float3 currentResult = max(0, accumulatedLight * volParams.z);
    
    // Protection against NaNs/Infs
    if (any(isnan(currentResult)) || any(isinf(currentResult))) {
        currentResult = float3(0, 0, 0);
    }
    
    // Reproject for history
    float4 prevClipPos = mul(float4(worldPos, 1.0f), prevViewProj);
    float3 prevScreenPos = prevClipPos.xyz / (prevClipPos.w + 0.00001f);
    float2 prevUV = prevScreenPos.xy * 0.5f + 0.5f;
    prevUV.y = 1.0f - prevUV.y;

    float3 history = historyTexture.SampleLevel(samLinear, prevUV, 0).rgb;
    if (any(isnan(history)) || any(isinf(history))) {
        history = currentResult;
    }
    
    float weight = volJitter.y;
    
    // Reduce weight during camera movement to prevent lag/ghosting
    float camMove = volJitter.z;
    weight *= saturate(1.0f - camMove * 10.0f);

    // TAA-style YCoCg Clipping
    float3 currYCoCg = RGBToYCoCg(currentResult);
    float3 histYCoCg = RGBToYCoCg(history);

    // Simple neighborhood estimate (1x1 for now, but with expanded box)
    float3 minColor = currYCoCg - float3(0.2f, 0.1f, 0.1f);
    float3 maxColor = currYCoCg + float3(0.2f, 0.1f, 0.1f);
    
    // Clamp history to current neighborhood in YCoCg space
    histYCoCg = clamp(histYCoCg, minColor, maxColor);
    history = YCoCgToRGB(histYCoCg);
    
    // Reject history if out of bounds
    if (prevUV.x < 0 || prevUV.x > 1 || prevUV.y < 0 || prevUV.y > 1) {
        weight = 0.0f;
    }

    float3 finalColor = lerp(currentResult, history, weight);

    return float4(finalColor, 1.0f);
}
