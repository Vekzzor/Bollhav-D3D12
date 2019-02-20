struct VertexIn
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
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
	float2 uv : TEXCOORD;
};

VertexOut vs_main(VertexIn input)
{
	VertexOut output;
	output.position   = float4(input.position, 1.0f) + pos;
	output.position   = mul(matIndex,output.position);
	output.uv		  = input.uv;
	return output;
}
