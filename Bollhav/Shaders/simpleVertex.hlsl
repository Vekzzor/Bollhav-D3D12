struct VertexIn
{
	float3 position : POSITION;
	//float3 normal : NORMAL;
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
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
	float3 worldPosition : POSITION;
};

VertexOut vs_main(VertexIn input)
{
	VertexOut output;
	output.position		 = float4(input.position, 1.0f);
	output.worldPosition = output.position.xyz;
	output.position		 = mul(matIndex, output.position);
	output.normal		 = float3(1, 1, 1);
	output.uv			 = float2(1, 0); //input.normal.xy);
	return output;
}
