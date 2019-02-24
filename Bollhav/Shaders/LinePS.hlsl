struct VertexOut
{
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION;
};

float4 PS_main(VertexOut input)
	: SV_TARGET
{
	
	if(input.worldPosition.z == 0 && input.worldPosition.y == 0 && input.worldPosition.x > 0)
		return float4(1, 0, 0, 1.0f);
	if(input.worldPosition.x == 0 && input.worldPosition.y == 0 && input.worldPosition.z > 0)
		return float4(0, 0, 1, 1.0f);
    if (input.worldPosition.x == 0 && input.worldPosition.z == 0 && input.worldPosition.y > 0)
        return float4(0, 1, 0, 1.0f);

	return float4(0,0,0, 1.0f);
		
}