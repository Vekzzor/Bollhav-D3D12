
struct DATA
{
    float3 pos;
    float3 vel;
};

RWStructuredBuffer<DATA> outputData : register(u0);
StructuredBuffer<DATA> srv : register(t0);

static float softeningSquared = 0.0012500000f * 0.0012500000f;
static float g_fG = 6.67300e-11f * 10000.0f;
static float g_fParticleMass = g_fG * 10000.0f * 10000.0f;

void bodyBodyInteraction(inout float3 ai, float3 bj, float3 bi, float mass, int particles)
{
    float3 r = bj.xyz - bi.xyz;

    float distSqr = dot(r, r);
    distSqr += softeningSquared;

    float invDist = 1.0f / sqrt(distSqr);
    float invDistCube = invDist * invDist * invDist;
    
    float s = mass * invDistCube * particles;

    ai += r * s;
}

[numthreads(1, 1, 1)]
void CS_main(uint3 DTid : SV_DispatchThreadID)
{
    uint index = DTid.x;
    DATA current = srv[index];
       
    float3 pos = current.pos;
    float3 vel = current.vel;
    float3 accel = 0;
    float mass = g_fParticleMass;
    
   
    for (uint i = 0; i < 1; i++)
    {
        bodyBodyInteraction(accel, srv[i].pos, pos, mass, 1);
 
    }
    vel.xyz += accel.xyz * 0.000001f; //deltaTime;
    vel.xyz *= 1; //damping;
    pos.xyz += vel.xyz * 0.0001f; //deltaTime;
   // pos.xyz += (accel.xyz * 0.001f) / 2.0f;
  
    outputData[index].pos = srv[index].pos;
    outputData[index].vel = vel;
    
}