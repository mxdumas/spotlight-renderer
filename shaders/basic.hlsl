cbuffer MatrixBuffer : register(b0) {
    matrix world;
    matrix view;
    matrix projection;
};

cbuffer SpotlightBuffer : register(b1) {
    float3 lightPos;
    float lightRange;
    float3 lightDir;
    float spotAngle;
    float3 lightColor;
    float lightIntensity;
    float2 beamAngles; // x: beam, y: field
    float2 padding;
};

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
    float3 toLight = lightPos - input.worldPos;
    float dist = length(toLight);
    toLight /= dist; // Normalize
    
    // Attenuation (Inverse Square Law)
    float attenuation = lightIntensity / (dist * dist + 1.0f);
    
    // Spotlight effect
    float cosAngle = dot(-toLight, normalize(lightDir));
    float spotEffect = saturate((cosAngle - beamAngles.y) / (beamAngles.x - beamAngles.y));
    spotEffect = pow(spotEffect, 2.0f); // Smoother falloff
    
    // Lambertian diffuse
    float diff = max(dot(normal, toLight), 0.0f);
    
    float3 ambient = float3(0.05f, 0.05f, 0.05f);
    float3 finalColor = (diff * lightColor * attenuation * spotEffect) + ambient;
    
    return float4(finalColor, 1.0f);
}