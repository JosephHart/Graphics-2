
//
// Model a simple light
//

// Ensure matrices are row-major
#pragma pack_matrix(row_major)


//-----------------------------------------------------------------
// Structures and resources
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Globals
//-----------------------------------------------------------------

cbuffer basicCBuffer : register(b0) {

	float4x4			worldViewProjMatrix;
	float4x4			worldITMatrix; // Correctly transform normals to world space
	float4x4			worldMatrix;
	float4				eyePos;
	float4				lightVec; // w=1: Vec represents position, w=0: Vec  represents direction.
	float4				lightAmbient;
	float4				lightDiffuse;
	float4				lightSpecular;
	float4				windDir;
	float				Timer;
	float				grassHeight;
};


//
// Textures
//

// Assumes texture bound to texture t0 and sampler bound to sampler s0
Texture2D myTexture : register(t0);
SamplerState linearSampler : register(s0);




//-----------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------

// Input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct FragmentInputPacket {

	// Vertex in world coords
	float3				posW			: POSITION;
	// Normal in world coords
	float3				normalW			: NORMAL;
	float4				matDiffuse		: DIFFUSE; // a represents alpha.
	float4				matSpecular		: SPECULAR; // a represents specular power. 
	float2				texCoord		: TEXCOORD;
	float4				posH			: SV_POSITION;
};


struct FragmentOutputPacket {

	float4				fragmentColour : SV_TARGET;
};


FragmentOutputPacket main(FragmentInputPacket v)
{
	FragmentOutputPacket outputFragment;

	outputFragment.fragmentColour = float4(0.0, 0.0, 0.0, 0.0);
	return outputFragment;
}