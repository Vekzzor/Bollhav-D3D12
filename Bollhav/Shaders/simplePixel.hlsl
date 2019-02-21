struct VertexOut
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
	float3 worldPosition : POSITION;
};

float4 PS_main(VertexOut input)
	: SV_TARGET
{
	const float3 lightPos = float3(10, 10, 0);
	float3 toLight		  = lightPos - input.worldPosition;


	float3 lightDir = float3(1, 0, 0);
	float angle		= max(dot(input.normal, normalize(toLight)), 0.0f);
	float3 color	= float3(1, 1, 1);
	color *= angle;
	if(input.worldPosition.z == 0 && input.worldPosition.y == 0 && input.worldPosition.x > 0)
		return float4(1, 0, 0, 1.0f);
	if(input.worldPosition.x == 0 && input.worldPosition.y == 0 && input.worldPosition.z > 0)
		return float4(0, 0, 1, 1.0f);

	return float4(0,0,0, 1.0f);

	/*if(input.worldPosition.x > 0 && input.worldPosition.x < 1 && !(input.worldPosition.z > 0 && input.worldPosition.z < 1))
		return float4(1,0,0, 1.0f);
	
	else if(!(input.worldPosition.x > 0 && input.worldPosition.x < 1) &&
			(input.worldPosition.z > 0 && input.worldPosition.z < 1))
		return float4(0, 0, 1, 1.0f);
	else 
		return float4(0, 0, 0, 1.0f);*/
		
}