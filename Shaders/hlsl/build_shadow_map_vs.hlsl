

// Ensure matrices are row-major
#pragma pack_matrix(row_major)

//-----------------------------------------------------------------
// Globals
//-----------------------------------------------------------------

cbuffer basicCBuffer : register(b0) {

	float4x4			worldViewProjMatrix;
	float4x4			worldITMatrix; // Correctly transform normals to world space
	float4x4			worldMatrix;
	float4x4			shadowMatrix;
	float4				eyePos;
	float4				lightVec; // w=1: Vec represents position, w=0: Vec  represents direction.
	float4				lightAmbient;
	float4				lightDiffuse;
	float4				lightSpecular;
	float4				windDir;
	float				Timer;
	float				grassHeight;

};



//-----------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------
struct vertexInputPacket {

	float3				posL		: POSITION;
	float3				normalL		: NORMAL;
	float4				matDiffuse	: DIFFUSE; // a represents alpha.
	float4				matSpecular	: SPECULAR;  // a represents specular power. 
	float2				Tex			: TEXCOORD;
};


struct vertexOutputPacket {


	// Vertex in world coords
	float3				posW			: POSITION;
	// Normal in world coords
	float3				normalW			: NORMAL;
	float4				matDiffuse		: DIFFUSE;
	float4				matSpecular		: SPECULAR;
	float2				Tex				: TEXCOORD;
	float4				posH			: SV_POSITION;
};


//-----------------------------------------------------------------
// Vertex Shader
//-----------------------------------------------------------------
vertexOutputPacket main(vertexInputPacket vin)
{
	vertexOutputPacket vout;

	vout.posH = mul(float4(vin.posL, 1.0f), worldViewProjMatrix);
	vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), shadowMatrix).xy;
	//vout.Tex = float4(vin.Tex, 0.0f, 1.0f).xy;

	// Lighting is calculated in world space.
	vout.posW = mul(float4(vin.posL, 1.0f), worldMatrix).xyz;
	// Transform normals to world space with gWorldIT.
	vout.normalW = mul(float4(vin.normalL, 1.0f), worldITMatrix).xyz;
	// Pass through material properties
	vout.matDiffuse = vin.matDiffuse;
	vout.matSpecular = vin.matSpecular;

	return vout;
}
