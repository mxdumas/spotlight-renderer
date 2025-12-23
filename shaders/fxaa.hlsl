// FXAA 3.11 - Fast Approximate Anti-Aliasing
// Based on Timothy Lottes' FXAA implementation (NVIDIA)

cbuffer FXAABuffer : register(b0) {
    float2 rcpFrame;    // 1.0 / screenSize
    float2 padding;
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

// FXAA quality settings
#define FXAA_EDGE_THRESHOLD 0.166
#define FXAA_EDGE_THRESHOLD_MIN 0.0833
#define FXAA_SUBPIX 0.75
#define FXAA_SUBPIX_CAP 0.75
#define FXAA_SEARCH_STEPS 10
#define FXAA_SEARCH_ACCELERATION 1

float FxaaLuma(float3 rgb) {
    return rgb.g * (0.587 / 0.299) + rgb.r;
}

float4 PS(PS_INPUT input) : SV_Target {
    float2 uv = input.uv;
    float2 posM = uv;

    // Sample center and neighbors
    float3 rgbM = inputTexture.Sample(samLinear, posM).rgb;
    float3 rgbN = inputTexture.Sample(samLinear, posM + float2(0, -rcpFrame.y)).rgb;
    float3 rgbS = inputTexture.Sample(samLinear, posM + float2(0, rcpFrame.y)).rgb;
    float3 rgbE = inputTexture.Sample(samLinear, posM + float2(rcpFrame.x, 0)).rgb;
    float3 rgbW = inputTexture.Sample(samLinear, posM + float2(-rcpFrame.x, 0)).rgb;

    float lumaM = FxaaLuma(rgbM);
    float lumaN = FxaaLuma(rgbN);
    float lumaS = FxaaLuma(rgbS);
    float lumaE = FxaaLuma(rgbE);
    float lumaW = FxaaLuma(rgbW);

    float rangeMin = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));
    float rangeMax = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));
    float range = rangeMax - rangeMin;

    // Early exit if no edge detected
    if (range < max(FXAA_EDGE_THRESHOLD_MIN, rangeMax * FXAA_EDGE_THRESHOLD)) {
        return float4(rgbM, 1.0);
    }

    // Sample corners
    float3 rgbNW = inputTexture.Sample(samLinear, posM + float2(-rcpFrame.x, -rcpFrame.y)).rgb;
    float3 rgbNE = inputTexture.Sample(samLinear, posM + float2(rcpFrame.x, -rcpFrame.y)).rgb;
    float3 rgbSW = inputTexture.Sample(samLinear, posM + float2(-rcpFrame.x, rcpFrame.y)).rgb;
    float3 rgbSE = inputTexture.Sample(samLinear, posM + float2(rcpFrame.x, rcpFrame.y)).rgb;

    float lumaNW = FxaaLuma(rgbNW);
    float lumaNE = FxaaLuma(rgbNE);
    float lumaSW = FxaaLuma(rgbSW);
    float lumaSE = FxaaLuma(rgbSE);

    float lumaNS = lumaN + lumaS;
    float lumaWE = lumaW + lumaE;
    float lumaLeftCorners = lumaNW + lumaSW;
    float lumaRightCorners = lumaNE + lumaSE;
    float lumaTopCorners = lumaNW + lumaNE;
    float lumaBottomCorners = lumaSW + lumaSE;

    // Compute edge direction
    float edgeHorz = abs(lumaTopCorners - 2.0 * lumaN) + abs(lumaWE - 2.0 * lumaM) * 2.0 + abs(lumaBottomCorners - 2.0 * lumaS);
    float edgeVert = abs(lumaLeftCorners - 2.0 * lumaW) + abs(lumaNS - 2.0 * lumaM) * 2.0 + abs(lumaRightCorners - 2.0 * lumaE);
    bool isHorizontal = edgeHorz >= edgeVert;

    // Select edge orientation and step
    float luma1 = isHorizontal ? lumaN : lumaW;
    float luma2 = isHorizontal ? lumaS : lumaE;
    float gradient1 = luma1 - lumaM;
    float gradient2 = luma2 - lumaM;

    bool is1Steepest = abs(gradient1) >= abs(gradient2);
    float gradientScaled = 0.25 * max(abs(gradient1), abs(gradient2));

    float stepLength = isHorizontal ? rcpFrame.y : rcpFrame.x;
    float lumaLocalAvg = 0.0;

    if (is1Steepest) {
        stepLength = -stepLength;
        lumaLocalAvg = 0.5 * (luma1 + lumaM);
    } else {
        lumaLocalAvg = 0.5 * (luma2 + lumaM);
    }

    float2 currentUV = posM;
    if (isHorizontal) {
        currentUV.y += stepLength * 0.5;
    } else {
        currentUV.x += stepLength * 0.5;
    }

    // Search along edge
    float2 offset = isHorizontal ? float2(rcpFrame.x, 0) : float2(0, rcpFrame.y);

    float2 uv1 = currentUV - offset;
    float2 uv2 = currentUV + offset;

    float lumaEnd1 = FxaaLuma(inputTexture.Sample(samLinear, uv1).rgb) - lumaLocalAvg;
    float lumaEnd2 = FxaaLuma(inputTexture.Sample(samLinear, uv2).rgb) - lumaLocalAvg;

    bool reached1 = abs(lumaEnd1) >= gradientScaled;
    bool reached2 = abs(lumaEnd2) >= gradientScaled;
    bool reachedBoth = reached1 && reached2;

    if (!reached1) uv1 -= offset;
    if (!reached2) uv2 += offset;

    if (!reachedBoth) {
        for (int i = 2; i < FXAA_SEARCH_STEPS; i++) {
            if (!reached1) {
                lumaEnd1 = FxaaLuma(inputTexture.Sample(samLinear, uv1).rgb) - lumaLocalAvg;
            }
            if (!reached2) {
                lumaEnd2 = FxaaLuma(inputTexture.Sample(samLinear, uv2).rgb) - lumaLocalAvg;
            }

            reached1 = abs(lumaEnd1) >= gradientScaled;
            reached2 = abs(lumaEnd2) >= gradientScaled;
            reachedBoth = reached1 && reached2;

            if (!reached1) uv1 -= offset * FXAA_SEARCH_ACCELERATION;
            if (!reached2) uv2 += offset * FXAA_SEARCH_ACCELERATION;

            if (reachedBoth) break;
        }
    }

    // Compute final offset
    float distance1 = isHorizontal ? (posM.x - uv1.x) : (posM.y - uv1.y);
    float distance2 = isHorizontal ? (uv2.x - posM.x) : (uv2.y - posM.y);

    bool isDirection1 = distance1 < distance2;
    float distanceFinal = min(distance1, distance2);
    float edgeThickness = distance1 + distance2;

    float pixelOffset = -distanceFinal / edgeThickness + 0.5;

    bool isLumaCenterSmaller = lumaM < lumaLocalAvg;
    bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0) != isLumaCenterSmaller;
    float finalOffset = correctVariation ? pixelOffset : 0.0;

    // Subpixel antialiasing
    float lumaAvg = (1.0 / 12.0) * (2.0 * lumaNS + 2.0 * lumaWE + lumaLeftCorners + lumaRightCorners);
    float subPixOffset = saturate(abs(lumaAvg - lumaM) / range);
    float subPixOffsetFinal = (-2.0 * subPixOffset + 3.0) * subPixOffset * subPixOffset;
    subPixOffsetFinal = subPixOffsetFinal * subPixOffsetFinal * FXAA_SUBPIX;

    finalOffset = max(finalOffset, subPixOffsetFinal);

    // Apply offset and sample
    float2 finalUV = posM;
    if (isHorizontal) {
        finalUV.y += finalOffset * stepLength;
    } else {
        finalUV.x += finalOffset * stepLength;
    }

    float3 rgbF = inputTexture.Sample(samLinear, finalUV).rgb;
    return float4(rgbF, 1.0);
}
