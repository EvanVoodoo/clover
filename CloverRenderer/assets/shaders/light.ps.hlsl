#define MAX_LIGHTS 16

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

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
    float4 pixel = float4(0.1, 0.1, 0.1, 1); // ambient light
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
            float dist = length(lightWorld - fragWorld);
            
            // inverse-square core (with the singularity guard) 
            float atten = 1.0 / (dist * dist + 1.0);
            
            // radius window that forces it to actually reach zero
            float radius = length(screenSize);
            float window = saturate(1.0 - dist / radius);
            window *= window;

            atten *= window;

            pixel.rgb += l.color * l.intensity * atten;
        }
        else
        {
            // Handle other light types if needed
        }
    }
    return pixel;
}
