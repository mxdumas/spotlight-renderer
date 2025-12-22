cbuffer MatrixBuffer : register(b0) {
    matrix world;
    matrix view;
    matrix projection;
};

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
    
    float4 worldPos = mul(float4(input.pos, 1.0f), world);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, projection);
    
    output.normal = mul(input.normal, (float3x3)world);
    output.uv = input.uv;
    
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
    float3 normal = normalize(input.normal);
    
    float diff = max(dot(normal, lightDir), 0.0f);
    float3 diffuse = diff * float3(1.0f, 1.0f, 1.0f);
    float3 ambient = float3(0.1f, 0.1f, 0.1f);
    
    return float4(diffuse + ambient, 1.0f);
}
