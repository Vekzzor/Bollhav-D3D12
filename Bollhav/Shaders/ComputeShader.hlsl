RWStructuredBuffer<float> outputData : register(u0);
StructuredBuffer<float> srv : register(t0);
[numthreads(1, 1, 1)]
void CS_main(uint3 DTid : SV_DispatchThreadID)
{
    outputData[DTid.x] = srv[0];
    
}