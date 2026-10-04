cbuffer ObjectBuffer : register(b0)
{
    matrix mvp;
    uint entityID;
    float3 _pad;
};

float4 ColorVertexShader(float3 position : POSITION) : SV_POSITION
{
    return mul(mvp, float4(position, 1.0f));
}