
struct DATA
{
	float3 pos;
	float3 vel;
};

cbuffer cbCS : register(b0)
{
	//x == nCubes
	//y == ceil(nCubes / blocksize)
	uint2 g_uiParams;
	//x == timeStep
	//y == damping
	float2 g_fParams;
	float centerM;
};

#define blocksize 128
groupshared float3 sharedPos[blocksize];

RWStructuredBuffer<DATA> outputData : register(u0);
StructuredBuffer<DATA> srv : register(t0);

static float softeningSquared = 0.0012500000f * 0.0012500000f;

static float defMass  = 100000.0f;


static float g_fG = 6.67300e-11f;
static float g_fParticleMass = g_fG * defMass * defMass;

void bodyBodyInteraction(inout float3 ai, float3 bj, float3 bi, float mass, int particles)
{
	//dir
	float3 r	  = bj.xyz - bi.xyz;
	float distSqr = dot(r, r);
	distSqr += softeningSquared;

	float invDist	 = 1.0f / sqrt(distSqr);
	float invDistCube = invDist * invDist * invDist;

	float s = mass * invDistCube * particles;
	ai += r * s;
}

[numthreads(blocksize, 1, 1)] void CS_main(uint3 DTid
										   : SV_DispatchThreadID, uint GIndex
										   : SV_GroupIndex) {
	uint index   = DTid.x;
	int cubeGroups = g_uiParams.y;
	float timestep = g_fParams.x;

	DATA current = srv[index];	
	float3 pos   = current.pos;
	float3 vel   = current.vel;

	float3 accel = 0;
	float mass   = g_fParticleMass;
	float centerMass = centerM * g_fParticleMass;

	[loop] for(uint i = 0; i < cubeGroups; i++)
	{
		sharedPos[GIndex] = srv[i * blocksize + GIndex].pos;
		GroupMemoryBarrierWithGroupSync();

		[unroll] for(uint counter = 0; counter < blocksize; counter += 1)
		{
			bodyBodyInteraction(accel, sharedPos[counter], pos, mass, 1);
		}

		GroupMemoryBarrierWithGroupSync();
	}
	bodyBodyInteraction(accel, float3(0, 0, 0), pos, centerMass, 1);
	
	const int tooManyParticles = g_uiParams.y * blocksize - g_uiParams.x;
	bodyBodyInteraction(accel, float4(0, 0, 0, 0), pos, mass, -tooManyParticles);

	vel.xyz += accel.xyz * timestep; //deltaTime;
	vel.xyz *= g_fParams.y; //damping;
	pos.xyz += vel.xyz * timestep; //deltaTime;

	outputData[index].pos = pos;
	outputData[index].vel = vel;
}