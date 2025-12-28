cbuffer MatrixBuffer : register(b0) {
    matrix world;
    matrix viewProj;   // Combined light view-projection matrix
    matrix padding1;   // Kept for layout compatibility
    matrix padding2;
    float4 cameraPos;
};

struct VS_INPUT {
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    float4 worldPos = mul(float4(input.pos, 1.0f), world);
    output.pos = mul(worldPos, viewProj);
    return output;
}

void PS(PS_INPUT input) {
    // Depth only pass, no output needed
}
