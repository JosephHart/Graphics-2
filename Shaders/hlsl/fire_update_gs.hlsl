
//
// Model the update of each particle.  Each particle is modelled in world coordinate space
//

// Resources
//-----------------------------------------------------------------
// Globals
//-----------------------------------------------------------------

cbuffer basicCBuffer : register(b0) {

	float4x4			viewProjMatrix;
	float4x4			worldITMatrix;				// Not used
	float4x4			worldMatrix;				// Not used
	float4x4			gLightWorldViewProjTexture;
	float4				eyePos;
	float4				lightVec;					// Not used
	float4				lightAmbient;				// Not used
	float4				lightDiffuse;				// Not used
	float4				lightSpecular;				// Not used
	float4				windDir;					// Not used
	float				Timer;
};


// Input / Output structures


struct ParticleStructure {
		float3 pos : POSITION;   // in object space
		float3 posL : LPOS;   // in object space
		float3 vel :VELOCITY;   // in object space
		float3 data : DATA;
};


//One particle in maps to 1 paricleout
[maxvertexcount(1)]
void main(point ParticleStructure inputParticle[1],
	inout PointStream<ParticleStructure> outputParticleStream) {

	ParticleStructure			outputParticle = inputParticle[0];

	//tweakable parameters
	float gPartLife = 1.2;
	float3 gPartSpeed = float3(4,8,4);

	float dtime = Timer;
	float3 g = float3(0, -3.9, 0);
	float3 wind = windDir.xyz*2.0;

	// Calculate force
	float3 force = g + wind; 

	outputParticle.vel = inputParticle[0].vel + dtime*force; //Update velocity
	outputParticle.pos = inputParticle[0].pos + (outputParticle.vel* dtime*gPartSpeed); // Update position
	outputParticle.data.x += dtime; //increment age
	
	//Check if particle has expired and respawn
	if (outputParticle.data.x > gPartLife)
	{
		outputParticle.data.x = 0;// fmod(outputParticle.data.x, gPartLife);
		outputParticle.vel = outputParticle.posL;// Copy back original velocity
		outputParticle.vel.y = 0.7+(abs(outputParticle.vel.x) + abs(outputParticle.vel.z))/2;
		outputParticle.pos = float3(0, 0, 0);
	}

	//Check if particle hits floor and bounce
	if (outputParticle.pos.y<0)
	{
		outputParticle.pos.y = -outputParticle.pos.y;
		outputParticle.vel.y = -outputParticle.vel.y*0.3;
		outputParticle.data.x += 0.1;
	}

	outputParticleStream.Append(outputParticle);
}
