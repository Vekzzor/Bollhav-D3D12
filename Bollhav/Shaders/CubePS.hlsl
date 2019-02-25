struct VertexOut
{
 
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
};

float4 PS_main(VertexOut input)
	: SV_TARGET
{
	
    return float4(input.normal, 1.0f);
		
}