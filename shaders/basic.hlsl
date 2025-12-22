struct VS_INPUT {
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    // For now, no transformations, just pass through
    output.pos = float4(input.pos, 1.0f);
    output.normal = input.normal;
    output.uv = input.uv;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    // Debug: visualize normals
    return float4(input.normal * 0.5f + 0.5f, 1.0f);
}