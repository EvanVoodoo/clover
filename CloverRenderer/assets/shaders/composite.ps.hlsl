Texture2D sceneTex : register(t0);
Texture2D lightTex : register(t1);
SamplerState samp : register(s0);

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
    float3 scene = sceneTex.Sample(samp, input.uv).rgb;
    float3 light = lightTex.Sample(samp, input.uv).rgb;
    return float4(scene * light, 1.0); // the multiply IS the lighting
}