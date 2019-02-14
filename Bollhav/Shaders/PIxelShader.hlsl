struct VSOut
{
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv : TEXCOORD;
};

float4 PS_main( VSOut input ) : SV_TARGET0
{
	return float4(1.0f, 0.0f,0.0f, 0.0f); 
}