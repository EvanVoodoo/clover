struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
    float2 offset = float2(0.005, 0.005);

    float r = texture0.Sample(sampler0, input.uv + offset).r;
    float g = texture0.Sample(sampler0, float2(input.uv.x - offset.x, input.uv.y + offset.y)).g;
    float b = texture0.Sample(sampler0, input.uv - offset).b;
    float a = texture0.Sample(sampler0, input.uv).a;

    return float4(r, g, b, a) * input.color;
}
