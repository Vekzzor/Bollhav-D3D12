struct VertexOut
{
 
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
	float3 velocity : COLOR;
};

float4 PS_main(VertexOut input)
	: SV_TARGET
{
	return float4(abs(input.velocity), 1.0f);	
}