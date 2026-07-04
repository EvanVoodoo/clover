Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

static const float warp = 0.75;
static const float scan = 0.75;

float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
    // Vignette from original UVs
    float2 vigUV = input.uv;
    vigUV *= 1.0 - vigUV.yx;
    float vignette = vigUV.x * vigUV.y * 15.0;
    vignette = pow(vignette, 0.25);

    // Warp from original UVs
    float2 uv = input.uv;
    float2 dc = abs(0.5 - uv);
    dc *= dc;
    
    uv.x -= 0.5;
    uv.x *= 1.0 + (dc.y * (0.3 * warp));
    uv.x += 0.5;
    uv.y -= 0.5;
    uv.y *= 1.0 + (dc.x * (0.4 * warp));
    uv.y += 0.5;

    float4 pixel;
    float4 texColor = texture0.Sample(sampler0, uv);

    if (uv.y > 1.0 || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0)
        pixel = float4(0.0, 0.0, 0.0, 1.0);
    else
    {
        float apply = abs(sin(input.position.y) * 0.5 * scan);
        pixel = float4(lerp(texColor.rgb, float3(0.0, 0.0, 0.0), apply) * vignette, 1.0);
    }

    return pixel;
}