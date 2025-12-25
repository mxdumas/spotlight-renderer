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

cbuffer MaterialBuffer : register(b2) {
    float4 matColor;
    float4 specParams; // x: intensity, y: shininess
};

struct PointLight {
    float4 pos;   // xyz: pos, w: range
    float4 color; // xyz: color, w: intensity
};

cbuffer CeilingLightsBuffer : register(b3) {
    PointLight pointLights[8];
    float4 ambientColor;
};

Texture2D goboTexture : register(t0);
Texture2D shadowMap : register(t1);
SamplerState samLinear : register(s0);
SamplerComparisonState shadowSampler : register(s1);

struct VS_INPUT {
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    float4 worldPos = mul(float4(input.pos, 1.0f), world);
    output.worldPos = worldPos.xyz;
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, projection);
    output.normal = mul(input.normal, (float3x3)world);
    output.uv = input.uv;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    float3 normal = normalize(input.normal);
    float3 viewDir = normalize(cameraPos.xyz - input.worldPos);

    float3 spotlighting = float3(0,0,0);

    // Loop through spotlights
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].colorInt.w <= 0.0f) continue;

        float3 toLight = lights[i].posRange.xyz - input.worldPos;
        float dist = length(toLight);
        toLight /= dist;

        // Attenuation
        float attenuation = lights[i].colorInt.w / (dist * dist + 1.0f);
        if (dist > lights[i].posRange.w) attenuation = 0;

        // Spotlight effect
        float3 LDir = normalize(lights[i].dirAngle.xyz);
        float cosAngle = dot(-toLight, LDir);

        float beam = lights[i].coneGobo.x;
        float field = lights[i].coneGobo.y;
        float spotEffect = saturate((cosAngle - field) / (max(0.001f, beam - field)));

        if (spotEffect > 0) {
             // Gobo & Shadow
            float3 goboColor = float3(0,0,0);
            float shadowFactor = 1.0f;
            
            // Project texture
            float4 lightSpacePos = mul(float4(input.worldPos, 1.0f), lights[i].lightViewProj);
            
            if (lightSpacePos.w > 0.0f) {
                float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
                float2 gUV = projCoords.xy;

                float s, c;
                sincos(lights[i].coneGobo.z, s, c);
                float2 rUV;
                rUV.x = gUV.x * c - gUV.y * s;
                rUV.y = gUV.x * s + gUV.y * c;
                rUV += lights[i].goboOff.xy;

                float2 finalUV = rUV * 0.5f + 0.5f;
                finalUV.y = 1.0f - finalUV.y;
                
                // Clamp gobo
                if (finalUV.x >= 0 && finalUV.x <= 1 && finalUV.y >= 0 && finalUV.y <= 1)
                     goboColor = goboTexture.Sample(samLinear, finalUV).rgb;

                // Shadow mapping (Only for first light for now)
                if (i == 0) {
                    float2 shadowUV = projCoords.xy * 0.5f + 0.5f;
                    shadowUV.y = 1.0f - shadowUV.y;
                    float depth = projCoords.z;

                    if (shadowUV.x >= 0 && shadowUV.x <= 1 && shadowUV.y >= 0 && shadowUV.y <= 1) {
                        shadowFactor = shadowMap.SampleCmpLevelZero(shadowSampler, shadowUV, depth - 0.0005f).r;
                    }
                }
            }

            float diff = max(dot(normal, toLight), 0.0f);
            float3 halfWay = normalize(toLight + viewDir);
            float spec = pow(max(dot(normal, halfWay), 0.0f), specParams.y) * specParams.x;

            spotlighting += (diff + spec) * lights[i].colorInt.xyz * attenuation * spotEffect * goboColor * shadowFactor;
        }
    }

    // Add Ceiling Lights
    float3 ceilingLighting = float3(0,0,0);
    for (int j = 0; j < 8; ++j) {
        float3 toPL = pointLights[j].pos.xyz - input.worldPos;
        float dPL = length(toPL);
        toPL /= dPL;
        float attPL = pointLights[j].color.w / (dPL * dPL + 1.0f);
        if (dPL > pointLights[j].pos.w) attPL = 0;

        float diffPL = max(dot(normal, toPL), 0.0f);
        float3 halfWayPL = normalize(toPL + viewDir);
        float specPL = pow(max(dot(normal, halfWayPL), 0.0f), specParams.y) * specParams.x;

        ceilingLighting += (diffPL + specPL) * pointLights[j].color.rgb * attPL;
    }

    float3 ambient = float3(0.001, 0.001, 0.001) + ambientColor.rgb; // Minimal base ambient + controllable fill

    return float4((spotlighting + ceilingLighting + ambient) * matColor.rgb, 1.0f);
}
