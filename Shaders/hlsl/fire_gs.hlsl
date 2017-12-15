
// Ensure matrices are row-major
#pragma pack_matrix(row_major)

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

//-----------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------

struct VertexInputPacket {
	float3 pos : POSITION;   // in object space
	float3 posL : LPOS;   // in object space
	float3 vel :VELOCITY;   // in object space
	float3 data : DATA;   // x=age
};

// Input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct VertexOutputPacket {

	float4 posH  : SV_POSITION;  // in clip space
	float2 texCoord  : TEXCOORD0;
	float4 Z  : DEPTH;
	float alpha : ALPHA;
};

//
// Geometry Shader
//


//Dynamic particle system
//Each vertex is expanded into a screen-aligned quad in view coordinate space.  
[maxvertexcount(8)]
void main(point VertexInputPacket inputVertex[1],
	inout TriangleStream<VertexOutputPacket> outputTriangleStream) {

	//tweakable parameters
	float gPartLife = 1.2;//seconds
	float gPartScale = 0.1;
	bool gDrawReflection = false;

	VertexOutputPacket		outputVertex;
	float age = inputVertex[0].data.x;
	outputVertex.alpha = 1 - (age / gPartLife);
	
	// Transform to world space.
	float3 pos = mul(float4(inputVertex[0].pos, 1), worldMatrix);
	// Compute world matrix so that billboard faces the camera.
	float3 look = normalize(eyePos - pos);
	float3 right = normalize(cross(float3(0, 1, 0), look));
	float3 up = cross(look, right);
	float2 posL[4];

	posL[0] = float2(1, 1);
	posL[1] = float2(1, -1);
	posL[2] = float2(-1, 1);
	posL[3] = float2(-1, -1);


	for (int i = 0; i < 4; i++)
	{
		float3 quadVertexPos = pos + (posL[i].x*right*gPartScale) + (posL[i].y*up*gPartScale);
		
		// Transform to homogeneous clip space.
		outputVertex.posH = mul(float4(quadVertexPos, 1.0f), viewProjMatrix);
		outputVertex.Z = outputVertex.posH;
		outputVertex.texCoord = float2((posL[i].x + 1)*0.5, (posL[i].y + 1)*0.5);
		outputTriangleStream.Append(outputVertex);
	}

	if (!gDrawReflection) return;
	outputTriangleStream.RestartStrip();

	// Add reflection inbn ground plane
	outputVertex.alpha = outputVertex.alpha / 4;

	// Transform to world space upside down.
	pos = mul(float4(inputVertex[0].pos*float3(1,-1,1), 1), worldMatrix);

	// Compute world matrix so that billboard faces the camera.
	look = normalize(eyePos - pos);
	right = normalize(cross(float3(0, 1, 0), look));
	up = cross(look, right);

	for (int i = 0; i < 4; i++)
	{
		float3 quadVertexPos = pos + (posL[i].x*right*gPartScale) + (posL[i].y*up*gPartScale);

		// Transform to homogeneous clip space.
		outputVertex.posH = mul(float4(quadVertexPos, 1.0f), viewProjMatrix);
		outputVertex.Z = outputVertex.posH;
		outputVertex.texCoord = float2((posL[i].x + 1)*0.5, (posL[i].y + 1)*0.5);
		outputTriangleStream.Append(outputVertex);
	}

}