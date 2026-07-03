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
        // rectangular to polar filter
        float2 norm = float2(input.uv.x, y / resolution.y) * 2.0 - 1.0;
        float theta = PI * 1.5 + norm.x * PI;
        float r = (1.0 + norm.y) * 0.5;

        // coord which we will sample from the occlusion map
        float2 coord = float2(-r * sin(theta), -r * cos(theta)) / 2.0 + 0.5;
        //coord.y = 1.0 - coord.y; // compensate for D3D's top-down V vs GL's bottom-up V

        // sample the occlusion map
        float4 data = texture0.Sample(sampler0, coord);

        // current distance is how far from the top we've come
        float dst = y / resolution.y;

        // if we've hit an opaque fragment (occluder), take the new distance
        // if it's less than what we have, use that for the ray
        float caster = data.a;
        if (caster > THRESHOLD)
        {
            dist = min(dist, dst);
        }
    }

    return float4(dist.xxx, 1.0);
}