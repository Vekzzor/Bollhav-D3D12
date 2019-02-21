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
	return float4(color, 1.0f);
}