Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
    float2 uv = input.uv;

    float4 pixel = texture0.Sample(sampler0, uv);
    return float4(pixel.r, pixel.g, pixel.b, pixel.a);
}