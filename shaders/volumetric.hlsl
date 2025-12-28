cbuffer MatrixBuffer : register(b0) {
    matrix world;
    matrix view;
    matrix projection;
    matrix invViewProj;
    float4 cameraPos;
};

struct SpotlightData {
    matrix lightViewProj;
    float4 posRange;      // xyz: pos, w: range
    float4 dirAngle;      // xyz: dir, w: spotAngle
    float4 colorInt;      // xyz: color, w: intensity
    float4 coneGobo;      // x: beam, y: field, z: rotation
    float4 goboOff;       // xy: offset
};

#define MAX_LIGHTS 4

cbuffer SpotlightBuffer : register(b1) {
    SpotlightData lights[MAX_LIGHTS];
};

cbuffer VolumetricBuffer : register(b2) {
    float4 volParams; // x: stepCount, y: density, z: intensity, w: anisotropy (G)
    float4 volJitter; // x: time
};

Texture2D depthTexture : register(t0);
Texture2DArray goboTexture : register(t1);
Texture2DArray shadowMap : register(t2);

SamplerState samLinear : register(s0);
SamplerComparisonState shadowSampler : register(s1);

struct VS_INPUT {
    float3 pos : POSITION;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f);
    return output;
}

float3 ScreenToWorld(float2 uv, float depth) {
    float4 clipPos = float4(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPos = mul(clipPos, invViewProj);
    return worldPos.xyz / worldPos.w;
}

float HenyeyGreenstein(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0f - g2) / (4.0f * 3.14159f * pow(max(0.001f, 1.0f + g2 - 2.0f * g * cosTheta), 1.5f));
}

// Interleaved Gradient Noise - deterministic, spatially coherent dither pattern
// Avoids temporal jitter while preventing banding
float InterleavedGradientNoise(float2 screenPos) {
    float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
    return frac(magic.z * frac(dot(screenPos, magic.xy)));
}

// Ray-Cone intersection
// Returns (t_enter, t_exit) or (-1, -1) if no intersection
// Cone defined by apex position, direction, and cosine of half-angle
float2 RayConeIntersect(float3 ray_origin, float3 ray_dir, float3 cone_apex, float3 cone_dir, float cos_angle, float max_range) {
    float3 co = ray_origin - cone_apex;  // Vector from apex to camera
    float co_len = length(co);

    float cos2 = cos_angle * cos_angle;
    float sin2 = 1.0f - cos2;

    // Check if camera is inside the cone
    // 1. Camera must be in front of apex (in cone direction)
    // 2. Angle from cone axis to camera must be less than cone angle
    bool in_front_of_apex = dot(co, cone_dir) > 0.0f;
    float cos_to_cam = (co_len > 0.0001f) ? dot(co / co_len, cone_dir) : 0.0f;
    bool inside_cone = in_front_of_apex && (cos_to_cam >= cos_angle) && (co_len < max_range);

    // Quadratic coefficients for ray-cone intersection
    float dv = dot(ray_dir, cone_dir);
    float cv = dot(co, cone_dir);
    float dd = dot(ray_dir, ray_dir);  // = 1 if normalized
    float cd = dot(co, ray_dir);
    float cc = dot(co, co);

    float a = dv * dv - cos2 * dd;
    float b = 2.0f * (dv * cv - cos2 * cd);
    float c = cv * cv - cos2 * cc;

    // Handle degenerate case: ray nearly parallel to cone surface
    if (abs(a) < 0.00001f) {
        if (inside_cone) {
            return float2(0.0f, max_range);
        }
        return float2(-1.0f, -1.0f);
    }

    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0.0f) {
        if (inside_cone) {
            return float2(0.0f, max_range);
        }
        return float2(-1.0f, -1.0f);
    }

    float sqrt_disc = sqrt(discriminant);
    float t1 = (-b - sqrt_disc) / (2.0f * a);
    float t2 = (-b + sqrt_disc) / (2.0f * a);

    // Ensure t1 < t2
    if (t1 > t2) {
        float tmp = t1;
        t1 = t2;
        t2 = tmp;
    }

    // Validate intersections: must be in front of camera AND in front half of cone
    float3 p1 = ray_origin + ray_dir * t1;
    float3 p2 = ray_origin + ray_dir * t2;

    bool valid1 = (t1 > 0.0f) && (dot(p1 - cone_apex, cone_dir) > 0.0f);
    bool valid2 = (t2 > 0.0f) && (dot(p2 - cone_apex, cone_dir) > 0.0f);

    if (inside_cone) {
        // Camera is inside cone - start at 0, find exit
        if (valid2) {
            return float2(0.0f, t2);
        } else if (valid1 && t1 > 0.0f) {
            return float2(0.0f, t1);
        } else {
            return float2(0.0f, max_range);
        }
    }

    // Camera outside cone
    if (valid1 && valid2) {
        return float2(t1, t2);
    } else if (valid1) {
        return float2(t1, max_range);
    } else if (valid2) {
        return float2(0.0f, t2);
    }

    return float2(-1.0f, -1.0f);
}

float4 PS(PS_INPUT input) : SV_Target {
    float2 uv = input.pos.xy / float2(1920.0f, 1080.0f);

    uint2 pixelPos = uint2(input.pos.xy);
    float depth = depthTexture.Load(uint3(pixelPos, 0)).r;

    float3 worldPos = ScreenToWorld(uv, depth);
    float3 camPos = cameraPos.xyz;

    if (depth >= 1.0f) {
        worldPos = ScreenToWorld(uv, 1.0f);
    }

    float3 rayDirVec = worldPos - camPos;
    float rayLen = length(rayDirVec);
    float3 rayDir = rayDirVec / max(rayLen, 0.0001f);

    float g = volParams.w;
    float totalStepCount = volParams.x;
    float3 accumulatedLight = float3(0, 0, 0);

    // Better noise: Interleaved Gradient Noise instead of white noise
    float noise = InterleavedGradientNoise(input.pos.xy);

    // Process each light with cone-aware marching
    [unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].colorInt.w <= 0.0f) continue;

        float3 LPos = lights[i].posRange.xyz;
        float3 LDir = normalize(lights[i].dirAngle.xyz);
        float range = lights[i].posRange.w;
        float beam = lights[i].coneGobo.x;
        float field = lights[i].coneGobo.y;

        // Use field angle (outer cone) for intersection
        // field is cos(angle), so the cone half-angle cosine
        float2 cone_t = RayConeIntersect(camPos, rayDir, LPos, LDir, field, range);

        // Skip if no valid intersection
        if (cone_t.x < 0.0f) continue;

        // Clamp to valid ray segment [0, rayLen] and light range
        float t_enter = max(0.0f, cone_t.x);
        float t_exit = min(rayLen, min(cone_t.y, range));

        // Skip if no valid segment
        if (t_enter >= t_exit) continue;

        float marchDist = t_exit - t_enter;

        // Use full step count per light - cone intersection already limits the march distance
        int stepCount = (int)totalStepCount;

        for (int s = 0; s < stepCount; ++s) {
            // Quadratic distribution: more samples near the light (t_enter is closer to light apex)
            float t_normalized = (float)s / (float)stepCount;
            // Invert quadratic: denser near light source (t_enter), sparser far (t_exit)
            // t_quad goes 0->1 but with more density at start
            float t_quad = 1.0f - (1.0f - t_normalized) * (1.0f - t_normalized);

            // Add jitter to reduce banding
            float jittered_t = t_quad + (noise - 0.5f) / (float)stepCount;
            jittered_t = saturate(jittered_t);

            float t = lerp(t_enter, t_exit, jittered_t);
            float3 currentPos = camPos + rayDir * t;

            // Calculate step length for integration (varies due to quadratic distribution)
            float next_t_normalized = (float)(s + 1) / (float)stepCount;
            float next_t_quad = 1.0f - (1.0f - next_t_normalized) * (1.0f - next_t_normalized);
            float stepLen = (next_t_quad - t_quad) * marchDist;

            float3 toLight = LPos - currentPos;
            float dist = length(toLight);

            if (dist < range) {
                float attenuation = lights[i].colorInt.w / (dist * dist + 1.0f);
                float3 toLightNorm = toLight / max(dist, 0.0001f);

                float cosAngle = dot(-toLightNorm, LDir);
                float spotEffect = saturate((cosAngle - field) / (max(0.001f, beam - field)));

                if (spotEffect > 0) {
                    float shadow = 1.0f;

                    // Shadow mapping
                    float4 lightSpacePos = mul(float4(currentPos, 1.0f), lights[i].lightViewProj);
                    float3 projCoords = float3(0, 0, 0);
                    if (lightSpacePos.w > 0.0f) {
                        projCoords = lightSpacePos.xyz / lightSpacePos.w;
                        float2 shadowUV = projCoords.xy * 0.5f + 0.5f;
                        shadowUV.y = 1.0f - shadowUV.y;

                        if (shadowUV.x >= 0 && shadowUV.x <= 1 && shadowUV.y >= 0 && shadowUV.y <= 1) {
                            shadow = shadowMap.SampleCmpLevelZero(shadowSampler, float3(shadowUV, i), projCoords.z - 0.01f).r;
                        }
                    }

                    if (shadow > 0) {
                        float cosTheta = dot(rayDir, -toLightNorm);
                        float phase = HenyeyGreenstein(cosTheta, g);

                        // Gobo sampling
                        float3 goboColor = float3(1,1,1);
                        {
                            float gs, gc;
                            sincos(lights[i].coneGobo.z, gs, gc);
                            float2 rUV;
                            rUV.x = projCoords.x * gc - projCoords.y * gs;
                            rUV.y = projCoords.x * gs + projCoords.y * gc;
                            rUV += lights[i].goboOff.xy;
                            float2 goboUV = rUV * 0.5f + 0.5f;
                            goboUV.y = 1.0f - goboUV.y;

                            if (goboUV.x >= 0 && goboUV.x <= 1 && goboUV.y >= 0 && goboUV.y <= 1)
                                goboColor = goboTexture.SampleLevel(samLinear, float3(goboUV, lights[i].coneGobo.w), 0).rgb;
                            else
                                goboColor = float3(0,0,0);
                        }

                        accumulatedLight += lights[i].colorInt.xyz * attenuation * spotEffect * shadow * goboColor * phase * volParams.y * stepLen;
                    }
                }
            }
        }
    }

    return float4(accumulatedLight * volParams.z, 1.0f);
}