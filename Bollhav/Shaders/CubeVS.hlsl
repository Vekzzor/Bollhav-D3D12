struct VertexIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

StructuredBuffer<float> test : register(t0);

cbuffer projection : register(b1)
{
    float4x4 matIndex;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
};

VertexOut vs_main(VertexIn input)
{
    VertexOut output;
    output.position = float4(input.position, 1.0f);
    output.position.x += test[0];
    output.worldPosition = output.position.xyz;
    output.position = mul(matIndex, output.position);
    output.normal = input.normal;
    return output;
}
