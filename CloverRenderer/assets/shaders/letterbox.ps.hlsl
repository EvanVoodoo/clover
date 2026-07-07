struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

cbuffer ResolutionBuffer : register(b0)
{
    float2 resolution;
    float2 padding; // pad to 16 bytes
};

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
    float2 uv = input.uv;
    
    float screenRatio = resolution.x / resolution.y;
    float targetRatio = 16.0 / 9.0;
    
    bool isLetterbox = screenRatio < targetRatio;
    
    float2 scale = isLetterbox ? float2(1.0, targetRatio / screenRatio) : float2(screenRatio / targetRatio, 1.0);
    
    float2 scaledUV = (uv - 0.5) * scale + 0.5;
    if (scaledUV.x < 0.0 || scaledUV.x > 1.0 || scaledUV.y < 0.0 || scaledUV.y > 1.0)
    {
        return float4(0, 0, 0, 1); // black letterbox bars
    }
    else
    {
        float4 texColor = texture0.Sample(sampler0, scaledUV);
        return texColor * input.color;
    }
}
