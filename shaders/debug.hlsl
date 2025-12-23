cbuffer MatrixBuffer : register(b0) {
    matrix world;
    matrix view;
    matrix projection;
};

struct VS_INPUT {
    float3 pos : POSITION;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    float4 worldPos = mul(float4(input.pos, 1.0f), world);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, projection);
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    return float4(1.0f, 0.0f, 0.0f, 1.0f); // Solid Red
}
