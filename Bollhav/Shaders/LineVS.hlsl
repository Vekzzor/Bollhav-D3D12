struct VertexIn
{
    float3 position : POSITION;
	//float3 normal : NORMAL;
};

cbuffer projection : register(b1)
{
    float4x4 matIndex;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION;
};

VertexOut vs_main(VertexIn input)
{
    VertexOut output;
    output.position = float4(input.position, 1.0f);
    output.worldPosition = output.position.xyz;
    output.position = mul(matIndex, output.position);
    return output;
}
