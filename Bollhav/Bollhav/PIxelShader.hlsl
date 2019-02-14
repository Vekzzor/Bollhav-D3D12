struct VSOut
{
	float4 pos		: SV_POSITION;
	float2 uv	: TEXCOORD;
	float4 normal : NOMRAL; 
};

float4 PS_main( VSOut input ) : SV_TARGET0
{
	return float4(1.0f, 0.0f,0.0f, 0.0f); 
}