#define MAX_LIGHTS 16
#define PI 3.14159265

struct Light
{
    float3 direction; // for directional; becomes position for point
    float intensity;
    float3 color;
    float type; // 0 = directional, 1 = point, etc. (float to respect packing)
};

cbuffer LightBuffer : register(b1)
{
    Light lights[MAX_LIGHTS];
    int lightCount;
    float3 _pad;
};

cbuffer M : register(b2)
{
    matrix invViewProj;
    float2 screenSize;
    float2 _pad2;
};

// Hardcoded shadow map slots -- one 1D distance texture per light
Texture2D shadowMap0 : register(t0);
Texture2D shadowMap1 : register(t1);
Texture2D shadowMap2 : register(t2);
Texture2D shadowMap3 : register(t3);
Texture2D shadowMap4 : register(t4);
Texture2D shadowMap5 : register(t5);
Texture2D shadowMap6 : register(t6);
Texture2D shadowMap7 : register(t7);
Texture2D shadowMap8 : register(t8);
Texture2D shadowMap9 : register(t9);
Texture2D shadowMap10 : register(t10);
Texture2D shadowMap11 : register(t11);
Texture2D shadowMap12 : register(t12);
Texture2D shadowMap13 : register(t13);
Texture2D shadowMap14 : register(t14);
Texture2D shadowMap15 : register(t15);
Texture2D occluderMask : register(t16);
SamplerState sampler0 : register(s0);

// matches lightSize used when building the occlusion/shadow maps in C++
static const float LIGHT_SIZE = 2048.0;

float SampleShadow(int index, float2 uv)
{
    if (index == 0)
        return shadowMap0.Sample(sampler0, uv).r;
    if (index == 1)
        return shadowMap1.Sample(sampler0, uv).r;
    if (index == 2)
        return shadowMap2.Sample(sampler0, uv).r;
    if (index == 3)
        return shadowMap3.Sample(sampler0, uv).r;
    if (index == 4)
        return shadowMap4.Sample(sampler0, uv).r;
    if (index == 5)
        return shadowMap5.Sample(sampler0, uv).r;
    if (index == 6)
        return shadowMap6.Sample(sampler0, uv).r;
    if (index == 7)
        return shadowMap7.Sample(sampler0, uv).r;
    if (index == 8)
        return shadowMap8.Sample(sampler0, uv).r;
    if (index == 9)
        return shadowMap9.Sample(sampler0, uv).r;
    if (index == 10)
        return shadowMap10.Sample(sampler0, uv).r;
    if (index == 11)
        return shadowMap11.Sample(sampler0, uv).r;
    if (index == 12)
        return shadowMap12.Sample(sampler0, uv).r;
    if (index == 13)
        return shadowMap13.Sample(sampler0, uv).r;
    if (index == 14)
        return shadowMap14.Sample(sampler0, uv).r;
    if (index == 15)
        return shadowMap15.Sample(sampler0, uv).r;
    return 1.0; // no shadow if out of range
}

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
    float4 pixel = float4(0.1, 0.1, 0.1, 1); // ambient light
    bool isOccluder = occluderMask.Sample(sampler0, input.uv).a > 0.5;
    for (int i = 0; i < lightCount; ++i)
    {
        Light l = lights[i];
        if (l.type == 0) // directional light
        {
            pixel += float4(l.color * l.intensity, 0);
        }
        else if (l.type == 1) // point light
        {
            // pixel coords -> [0,1] -> NDC
            float2 ndc = (input.position.xy / screenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y; // pixel-y grows down, NDC-y grows up

            // NDC -> world via inverse view-projection (row-vector convention)
            float4 worldPos = mul(float4(ndc, 0.0, 1.0), invViewProj);
            float2 fragWorld = worldPos.xy;

            // both in world space now
            float2 lightWorld = l.direction.xy; // overloaded as position
            float2 delta = fragWorld - lightWorld;
            float dist = length(delta);

            // inverse-square core (with the singularity guard)
            float atten = 1.0 / (dist * dist + 1.0);

            // radius window that forces it to actually reach zero
            float radius = length(screenSize);
            float window = saturate(1.0 - dist / radius);
            window *= window;

            atten *= window;

            float lit = 0.0;
            if (isOccluder)
            {
                lit = 0.001; // default-normal: no self-shadow test, distance attenuation only
            }
            else
            {
                // --- shadow lookup ---
                // normalized distance from light center, matching how the
                // occlusion/shadow-map pass was built (lightSize x lightSize ortho)
                float r = dist / (LIGHT_SIZE * 0.5);

                // angle of this fragment relative to the light, matched to
                // the polar transform used when building the 1D shadow map
                float theta = atan2(delta.y, delta.x);
                float tc = (theta + PI) / (2.0 * PI);

                float shadowDist = SampleShadow(i, float2(tc, 0.0));

                const float SHADOW_BIAS = 0.0;
                lit = step(r - SHADOW_BIAS, shadowDist);
            }

            atten *= lit;

            pixel.rgb += l.color * l.intensity * atten;
        }
        else
        {
            // Handle other light types if needed
        }
    }
    return pixel;
}