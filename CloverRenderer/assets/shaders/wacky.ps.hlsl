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
    float2 centered = input.uv - float2(0.5, 0.5);
    float dist = length(centered);
    float angle = dist * 0.2;
    float s = sin(angle);
    float c = cos(angle);
    float2 swirled = float2(
        centered.x * c - centered.y * s,
        centered.x * s + centered.y * c
    ) + float2(0.5, 0.5);

    float2 offset = centered * 0.02;
    float r = texture0.Sample(sampler0, swirled + offset).r;
    float g = texture0.Sample(sampler0, swirled).g;
    float b = texture0.Sample(sampler0, swirled - offset).b;

    float3 col = 1.0 - float3(r, g, b);
    col = floor(col * 4.0) / 4.0;

    return float4(col, 1.0) * input.color;
}
