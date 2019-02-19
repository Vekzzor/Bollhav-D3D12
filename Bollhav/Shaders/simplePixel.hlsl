struct VertexOut
{
	float4 position : SV_POSITION;
};

float4 PS_main(VertexOut input) : SV_TARGET
{
	return input.position;
}