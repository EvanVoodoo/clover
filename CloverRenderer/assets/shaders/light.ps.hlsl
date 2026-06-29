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
            float NdotL = saturate(dot(normalize(float3(0, 0, 1)), -normalize(l.direction)));
            pixel += float4(l.color * l.intensity * NdotL, 1);
        }
        else if (l.type == 1) // point light
        {
            float3 fragPos = float3(input.position.xy, 0);
            float3 lightDir = normalize(l.direction - fragPos);
            float distance = length(l.direction - fragPos);
            float attenuation = 1.0 / (distance * distance); // simple quadratic attenuation
            float NdotL = saturate(dot(normalize(float3(0, 0, 1)), lightDir));
            pixel += float4(l.color * l.intensity * NdotL * attenuation, 1);
        }
        else
        {
            // Handle other light types if needed
        }
    }
    return pixel;
}
