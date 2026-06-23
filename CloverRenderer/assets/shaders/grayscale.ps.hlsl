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
    float gray = dot(texColor.rgb, float3(0.299, 0.587, 0.114));
    gray *= 1.0;
    return float4(gray, gray, gray, texColor.a);
}
