struct VertexIn
{
	float3 position : POSITION;
};

cbuffer translate : register(b0)
{
	float4 pos;
};

cbuffer projection : register(b1)
{
	float4x4 matIndex;
};

struct VertexOut
{
	float4 position : SV_POSITION;
};

VertexOut vs_main(VertexIn input)
{
	VertexOut output;
	output.position   = float4(input.position, 1.0f) + pos;
	output.position.z = -5.0f;
	output.position   = mul(matIndex,output.position);
	return output;
}
