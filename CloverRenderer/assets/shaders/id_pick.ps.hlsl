cbuffer ObjectBuffer : register(b0)
{
    matrix mvp;
    uint entityID;
    float3 _pad;
};

float4 ColorPixelShader() : SV_TARGET
{
    uint id = entityID;
    
    // convert the ID to a color
    float r = ((id >> 16) & 0xFF) / 255.0f;
    float g = ((id >> 8) & 0xFF) / 255.0f;
    float b = ((id >> 0) & 0xFF) / 255.0f;

    return float4(r, g, b, 1.0f); // alpha = 1 means "something is here", regardless of ID value
}