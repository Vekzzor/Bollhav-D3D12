struct VSIn
{
	float3 pos		: POSITION;
	float3 normal : NORMAL; 
	float2 uv : TEXCOORD; 
};

struct VSOut
{
	float4 pos		: SV_POSITION;
	float4 normal : NORMAL;
	float2 uv : TEXCOORD;
};

cbuffer CB : register(b0)
{
	//Transfer data
}

VSOut VS_main( VSIn input, uint index : SV_VertexID )
{
	VSOut output	= (VSOut)0;
	output.pos		= float4( input.pos, 1.0f );
	output.uv = input.uv; 
	output.normal = float4(input.normal, 1.0f ); 

	return output;
}