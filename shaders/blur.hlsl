// Simple Gaussian Blur for volumetrics

cbuffer BlurBuffer : register(b0) {
    float2 texelSize;   // 1.0 / textureSize
    float2 direction;   // (1,0) for horizontal, (0,1) for vertical
};

Texture2D inputTexture : register(t0);
SamplerState samLinear : register(s0);

struct VS_INPUT {
    float3 pos : POSITION;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.uv = float2((input.pos.x + 1.0f) * 0.5f, (1.0f - input.pos.y) * 0.5f);
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    float2 uv = input.uv;

    // 5-tap Gaussian blur (subtle)
    float weights[3] = { 0.4, 0.25, 0.05 };

    float3 result = inputTexture.Sample(samLinear, uv).rgb * weights[0];

    for (int i = 1; i < 3; i++) {
        float2 offset = direction * texelSize * float(i);
        result += inputTexture.Sample(samLinear, uv + offset).rgb * weights[i];
        result += inputTexture.Sample(samLinear, uv - offset).rgb * weights[i];
    }

    return float4(result, 1.0f);
}
