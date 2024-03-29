
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
	float4x4			gLightWorldViewProjTexture;
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
	float4				matDiffuse		: DIFFUSE;
	float4				matSpecular		: SPECULAR;
	float2				texCoord		: TEXCOORD0;
	float4				ProjTex			: TEXCOORD1;
	float4				posH			: SV_POSITION;
	float3				TangentW		: TANGENT;
};


struct FragmentOutputPacket {

	float4				fragmentColour : SV_TARGET;
};


//-----------------------------------------------------------------
// Pixel Shader - Lighting 
//-----------------------------------------------------------------

FragmentOutputPacket main(FragmentInputPacket v) {

	FragmentOutputPacket outputFragment;

	//added
	// Complete projection by doing division by w.
	v.ProjTex.xyz /= v.ProjTex.w;
	// Depth in NDC space.
	float depth = v.ProjTex.z;
	// Sample the texture using the projective tex-coords.
	float4 c = myTexture.Sample(linearSampler, v.ProjTex.xy);
	//added

	float3 N = normalize(v.normalW);

	float4 baseColour = v.matDiffuse;

	baseColour = baseColour * c;

	////Initialise returned colour to ambient component
	float3 colour = baseColour.xyz* lightAmbient;

	// Calculate the lambertian term (essentially the brightness of the surface point based on the dot product of the normal vector with the vector pointing from v to the light source's location)
	float3 lightDir = -lightVec.xyz; // Directional light
	if (lightVec.w == 1.0) lightDir = lightVec.xyz - v.posW; // Positional light
	lightDir = normalize(lightDir);

	// Add diffuse light if relevant (otherwise we end up just returning the ambient light colour)
	colour += max(dot(lightDir, N), 0.0f) *baseColour.xyz * lightDiffuse;

	// Calc specular light
	float specPower = max(v.matSpecular.a*1000.0, 1.0f);

	float3 eyeDir = normalize(eyePos - v.posW);
	float3 R = reflect(-lightDir, N);
	float specFactor = pow(max(dot(R, eyeDir), 0.0f), specPower);
	colour += specFactor * v.matSpecular.xyz * lightSpecular;

	outputFragment.fragmentColour = float4(colour, baseColour.a);
	return outputFragment;

}
