struct VertexOut
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PS_main(VertexOut input) : SV_TARGET
{
	return float4(input.uv, 1.0f, 1.0f);
}