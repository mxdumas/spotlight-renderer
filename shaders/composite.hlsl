Texture2D volTexture : register(t0);
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
    output.uv = input.pos.xy * 0.5f + 0.5f;
    output.uv.y = 1.0f - output.uv.y;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    float3 color = volTexture.Sample(samLinear, input.uv).rgb;
    return float4(color, 1.0f);
}
