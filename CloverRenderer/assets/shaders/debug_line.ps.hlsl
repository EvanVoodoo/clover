struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
    return input.color;
}
