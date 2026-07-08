#define MAX_LIGHTS 64
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
    // matches lightSize used when building the occlusion/shadow maps in C++
    float lightSize;
    float zoom;
    float directionalLightSize; // directional light occlusion viewport size (the "size" you set to max(width,height))
    float2 camPosition; // main camera's world position this frame
    float2 _pad2;

};

Texture2D shadowMap : register(t0);
Texture2D occluderMask : register(t1);
SamplerState sampler0 : register(s0);

float SampleShadow(int index, float u)
{
    float v = (float(index) + 0.5) / float(MAX_LIGHTS);
    return shadowMap.Sample(sampler0, float2(u, v)).r;
    return 1.0; // no shadow if out of range
}

float Dither(float2 screenPos)
{
    return (frac(sin(dot(screenPos, float2(12.9898, 78.233))) * 43758.5453) - 0.5) / 255.0;
}

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
    float3 amb = float3(0.05, 0.05, 0.05);
    float4 pixel = float4(amb.rgb, 1); // ambient light
    bool isOccluder = occluderMask.Sample(sampler0, input.uv).a > 0.5;
    const float SHADOW_BIAS = 0.002;
    
    for (int i = 0; i < lightCount; ++i)
    {
        Light l = lights[i];
        if (l.type == 0) // directional light
        {
            // reconstruct this fragment's world position (same as point-light path)
            float2 ndc = float2(input.position.x / screenSize.x, input.position.y / screenSize.y) * 2.0 - 1.0;
            ndc.y = -ndc.y;
            float4 worldPos = mul(float4(ndc, 0.0, 1.0), invViewProj);
            float2 fragWorld = worldPos.xy;

            float2 dir = normalize(l.direction.xy); // light's forward axis
            float2 right = float2(-dir.y, dir.x); // perpendicular axis -- must match Transform's rotation convention

            float2 rel = fragWorld - camPosition;

            // project onto light-aligned axes, normalize into [0,1] to match the occlusion camera's ortho size
            float u = dot(rel, right) / lightSize + 0.5;
            float depth = dot(rel, dir) / lightSize + 0.5;

            float lit;
            if (isOccluder)
            {
                lit = 0.5; // same default-normal treatment as point lights
            }
            else
            {
                float shadowDepth = SampleShadow(i, u); // reuses the same shadowMap atlas, this light's row
                lit = step(depth - SHADOW_BIAS, shadowDepth);
            }

            pixel.rgb += l.color * l.intensity * lit;
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
            float dist = length(delta) * zoom;

            // inverse-square core (with the singularity guard)
            float atten = 1.0 / (dist * dist + 1.0);

            // radius window that forces it to actually reach zero
            float radius = length(lightSize);
            float window = saturate(1.0 - dist / radius);
            window *= window;

            atten *= window;

            float lit = 0.0;
            if (isOccluder)
            {
                lit = 0.01; // default-normal: no self-shadow test, distance attenuation only
            }
            else
            {
                // --- shadow lookup ---
                // normalized distance from light center, matching how the
                // occlusion/shadow-map pass was built (lightSize x lightSize ortho)
                float r = dist / (0.5 * lightSize);

                // angle of this fragment relative to the light, matched to
                // the polar transform used when building the 1D shadow map
                float theta = atan2(delta.y, delta.x);
                float tc = (theta + PI) / (2.0 * PI);

                float shadowDist = SampleShadow(i, tc);
                lit = step(r - SHADOW_BIAS, shadowDist);
            }

            atten *= lit * zoom * zoom;

            pixel.rgb += l.color * l.intensity * atten;
        }
        else
        {
            // Handle other light types if needed
        }
    }
    float ditherAmount = Dither(input.position.xy);
    pixel += float4(ditherAmount, ditherAmount, ditherAmount, 0);
    return pixel;
}