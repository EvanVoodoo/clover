struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer ResolutionBuffer : register(b0)
{
    float2 resolution;
    float2 padding; // pad to 16 bytes
};

static const float PI = 3.14;
static const float THRESHOLD = 0.75;

float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
    float dist = 1.0;
    for (float y = 0.0; y < resolution.y; y += 1.0)
    {
        float2 coord = float2(input.uv.x, y / resolution.y); // no polar remap needed
        float4 data = texture0.Sample(sampler0, coord);
        if (data.a > THRESHOLD)
        {
            float dst = y / resolution.y;
            dist = min(dist, dst);
        }
    }

    return float4(dist.xxx, 1.0);
}