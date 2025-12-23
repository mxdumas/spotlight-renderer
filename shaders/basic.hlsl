cbuffer MatrixBuffer : register(b0) {
    matrix world;
    matrix view;
    matrix projection;
};

cbuffer SpotlightBuffer : register(b1) {
    matrix lightViewProj;
    float3 lightPos;
    float lightRange;
    float3 lightDir;
    float spotAngle;
    float3 lightColor;
    float lightIntensity;
    float2 beamAngles; // x: beam, y: field
    float goboRotation;
    float goboShake;
};

Texture2D goboTexture : register(t0);
SamplerState samLinear : register(s0);

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
    float3 LDir = normalize(lightDir);
    float cosAngle = dot(-toLight, LDir);
    float spotEffect = saturate((cosAngle - beamAngles.y) / (beamAngles.x - beamAngles.y));
    
    // Gobo Projection
    float4 lightSpacePos = mul(float4(input.worldPos, 1.0f), lightViewProj);
    float2 goboUV = lightSpacePos.xy / lightSpacePos.w;
    
    // Rotation and Shake
    float s = sin(goboRotation);
    float c = cos(goboRotation);
    float2 rotatedUV;
    rotatedUV.x = goboUV.x * c - goboUV.y * s;
    rotatedUV.y = goboUV.x * s + goboUV.y * c;
    
    // Simple shake using sine wave (could use a time variable if we had one in cbuffer)
    // For now we use goboShake as a direct offset or we could add a time member to the buffer
    // Let's assume goboShake is the intensity of a random-ish jitter we calculate in C++
    rotatedUV += goboShake; // In this version, C++ should pass the calculated offset

    goboUV = rotatedUV * 0.5f + 0.5f; // [-1, 1] to [0, 1]
    goboUV.y = 1.0f - goboUV.y; // Flip Y for DirectX
    
    float3 goboColor = goboTexture.Sample(samLinear, goboUV).rgb;
    
    // Lambertian diffuse
    float diff = max(dot(normal, toLight), 0.0f);
    
    float3 ambient = float3(0.05f, 0.05f, 0.05f);
    float3 finalColor = (diff * lightColor * attenuation * spotEffect * goboColor) + ambient;
    
    return float4(finalColor, 1.0f);
}