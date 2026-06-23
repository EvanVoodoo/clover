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
    float4 texColor = texture0.Sample(sampler0, input.uv) * input.color;
    return float4(1.0 - texColor.rgb, texColor.a);
}
