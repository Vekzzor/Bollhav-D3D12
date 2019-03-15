struct VertexIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct DATA
{
    float3 pos;
    float3 vel;
};
StructuredBuffer<DATA> test : register(t0);

cbuffer projection : register(b1)
{
    float4x4 matIndex;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
	float3 velocity : COLOR;
};

VertexOut vs_main(in VertexIn input, in uint instanceID : SV_InstanceID)
{
    VertexOut output;
    output.position = float4(input.position, 1.0f);
    output.position.xyz += test[instanceID].pos.xyz;
    output.worldPosition = output.position.xyz;
    output.position = mul(matIndex, output.position);
    output.normal = input.normal;
	output.velocity = normalize(test[instanceID].vel);
    return output;
}
