struct VertexIn
{
	float3 position : POSITION;
};

struct VertexOut
{
	float4 position : SV_POSITION;
};

VertexOut vs_main(VertexIn input)
{
	VertexOut output;
	output.position = float4(input.position,1.0f);
	return output;
}
