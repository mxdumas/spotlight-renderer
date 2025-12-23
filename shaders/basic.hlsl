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

cbuffer MaterialBuffer : register(b2) {
    float4 matColor;
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
    float3 toLight = posRange.xyz - input.worldPos;
    float dist = length(toLight);
    toLight /= dist; 
    
    // Attenuation
    float attenuation = colorInt.w / (dist * dist + 1.0f);
    if (dist > posRange.w) attenuation = 0;
    
    // Spotlight effect
    float3 LDir = normalize(dirAngle.xyz);
    float cosAngle = dot(-toLight, LDir);
    
    float beam = coneGobo.x;
    float field = coneGobo.y;
    float spotEffect = saturate((cosAngle - field) / (max(0.001f, beam - field)));
    
    // Gobo & Shadow
    float3 goboColor = float3(0,0,0);
    float shadowFactor = 1.0f;
    float4 lightSpacePos = mul(float4(input.worldPos, 1.0f), lightViewProj);
    if (lightSpacePos.w > 0.0f) {
        float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
        float2 gUV = projCoords.xy;
        
        float s, c;
        sincos(coneGobo.z, s, c);
        float2 rUV;
        rUV.x = gUV.x * c - gUV.y * s;
        rUV.y = gUV.x * s + gUV.y * c;
        rUV += goboOff.xy;

        float2 finalUV = rUV * 0.5f + 0.5f;
        finalUV.y = 1.0f - finalUV.y;
        goboColor = goboTexture.Sample(samLinear, finalUV).rgb;

        // Shadow mapping
        float2 shadowUV = projCoords.xy * 0.5f + 0.5f;
        shadowUV.y = 1.0f - shadowUV.y;
        float depth = projCoords.z;
        
        if (shadowUV.x >= 0 && shadowUV.x <= 1 && shadowUV.y >= 0 && shadowUV.y <= 1) {
            shadowFactor = shadowMap.SampleCmpLevelZero(shadowSampler, shadowUV, depth - 0.0005f).r;
        }
    }

    float diff = max(dot(normal, toLight), 0.0f);
    float3 lighting = diff * colorInt.xyz * attenuation * spotEffect * goboColor * shadowFactor;
    float3 ambient = float3(0.01, 0.01, 0.01);
    
    return float4((lighting + ambient) * matColor.rgb, 1.0f);
}
